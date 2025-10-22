#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <logger.h>
#include <server.h>
#include <algorithm>
#include <request.h>
#include <exceptions.h>
#include <response.h>
#include <sstream>
#include <middleware.h>


using namespace std;
const int BACKLOG = 10; //How many pending connections queue will hold
extern vector<std::string> SUPPORTED_METHODS;

Server::Server(): logger(LOG_FILE, DEBUG) {
}

void Server::listenRequests(int serverSocketFd){

    while(true){
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSoc = accept(serverSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if(clientSoc == -1){
            continue;
        }

        //Handle new client connection
        struct sockaddr *clientAddrPtr = (struct sockaddr *)&clientAddr;
        int addressLen = clientAddrPtr->sa_family== AF_INET? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
        char ip[addressLen];
        getIpStr(clientAddrPtr, ip, addressLen);
        logger.logInfo("Accepted new connection from " + string(ip));


        //Start here to listen and respond to client requests
        //1. Use recv to get data from client
        string requestHeaders;
        string requestBody;
        int recvStatus = receiveHttpRequest(clientSoc, requestHeaders, requestBody);
        if(recvStatus <= 0){
            logger.logWarn("Failed to receive complete HTTP request from " + string(ip));
            //send a bad request response
            close(clientSoc);
            continue;
        }
        logger.logDebug("Received HTTP request headers:\n" + requestHeaders);
        logger.logDebug("Received HTTP request body:\n" + requestBody);

        //2. Parse the request and generate request object
        Request request(requestHeaders, requestBody);
        Response response(clientSoc);
        // logger.logInfo("Parsed HTTP request: " + request.method + " " + request.uri + " " + request.httpVersion);
        // for(auto p: request.queryParams){
        //     logger.logDebug("Key: " + p.first + " Value: " + p.second);
        // }

        // for(auto p: request.headers){
        //     logger.logDebug("Key: " + p.first + " Value: " + p.second);
        // }

        //3. Handle the middlewares here
        for(auto middlewareFunc: middelwareFunctions){
            middlewareFunc(request, response);
        }

        //4. Match the request to registered routes and do error handling if not found
        handlerFunction routeHandler = getRouteHandlerForPath(request);
        //Invalid request route
        if(!routeHandler){
            logger.logWarn("No handler found for route: " + request.path);
            //send 404 response
            
            response.setStatus(404);
            int sendStatus = response.sendResponse();
            if(sendStatus == 0){
                logger.logWarn("Failed to send response to client as client closed the connection" + string(ip));
            }else if(sendStatus < 0){
                logger.logError("Failed to send response to client " + string(ip));
            }
            close(clientSoc);
            continue;
        }


        //5. Call the respective handler functions and handler will send the response back
        routeHandler(request, response);

        if(response.sendResponseStatus == 0){
            logger.logWarn("Client closed the connection. IP: " + string(ip));
            close(clientSoc);
        }else if(response.sendResponseStatus == -1){
            logger.logWarn("Failed to send data to IP: " + string(ip));
            close(clientSoc);
        }
        logger.logInfo("Sent response to client " + string(ip));

        //6. Close the client socket based on the Connection header
        close(clientSoc);
    }
    
}

int Server::receiveHttpRequest(int clientSocketFd, string &requestHeaders,string &requestBody){
    string &headerBuffer = requestHeaders;
    string &bodyBuffer = requestBody;
    char tempBuffer[4096];
    bool headerReceived = false;
    size_t contentLength = 0;
    size_t totalBodyBytesReceived = 0;
    while(true){
        
        int bytesReceived = recv(clientSocketFd, tempBuffer, sizeof(tempBuffer) , 0);
        //Connection closed or error
        if(bytesReceived == 0){
            //0 indicating connection closed
            //only executes when headers are not yet received completely or body not fully received
            logger.logWarn("Connection closed before complete request was received");
            return -1;
        }else if(bytesReceived < 0){
            //-1 indicating error
            if(errno == EINTR){
                //Interrupted by signal, try again
                continue;
            }
            logger.logError("Receive error: " + string(strerror(errno)));
            return -1;
        }else{
            if(!headerReceived){
                headerBuffer.append(tempBuffer, bytesReceived);
                size_t headerEndPos = headerBuffer.find("\r\n\r\n");
                if(headerEndPos != string::npos){
                    headerReceived = true;
                    // Find where "Content-Length" starts
                    string lowerCaserHeader = headerBuffer.substr(0, headerEndPos);
                    
                    //handle case insensitivity
                    transform(lowerCaserHeader.begin(), lowerCaserHeader.end(), lowerCaserHeader.begin(), ::tolower);
                    size_t contentPos = lowerCaserHeader.find("content-length:", 0);
                    if (contentPos != string::npos) {
                        // find end of that line (\r\n)
                        size_t contentEnd = lowerCaserHeader.find("\r\n", contentPos);
                        
                        // extract substring between the colon and \r\n
                        string lenStr = lowerCaserHeader.substr(contentPos + 15, contentEnd - (contentPos + 15)); 
                        // 15 = length of "Content-Length:" including colon

                        // trim whitespace (just in case)
                        size_t first = lenStr.find_first_not_of(" \t");
                        size_t last  = lenStr.find_last_not_of(" \t");
                        if (first != std::string::npos)
                            lenStr = lenStr.substr(first, last - first + 1);
                        else
                            lenStr.clear();
                        
                        try{
                            contentLength = stoul(lenStr);
                            if(contentLength <= 0){
                                //No body to receive
                                bodyBuffer = "";
                                return 1;
                            }
                        }catch(exception& e){
                            logger.logWarn("Invalid Content-Length value: " + lenStr);
                            return -2;
                        }
                    }else{
                        // Content-Length header not found, assuming no body
                        bodyBuffer = "";
                        return 1;
                    }

                    // Move any extra data (part of body) to bodyBuffer
                    size_t bodyStartPos = headerEndPos + 4; // 4 is length of "\r\n\r\n"
                    if(headerBuffer.length() > bodyStartPos){
                        bodyBuffer = headerBuffer.substr(bodyStartPos);
                        totalBodyBytesReceived = bodyBuffer.length();
                    }
                    // Keep only headers in headerBuffer. Remove last \r\n\r\n
                    headerBuffer.erase(headerEndPos); 

                }
            }else{
                bodyBuffer.append(tempBuffer, bytesReceived);
                totalBodyBytesReceived += bytesReceived;
            }
            if(totalBodyBytesReceived == contentLength){
                return 1;
            }else if(totalBodyBytesReceived > contentLength){
                //Received more than expected
                //Bad request
                return -2;
            }
            continue; 

        }

        //handle size limits. Saves from DoS attacks
        if (headerBuffer.size() > 8192) {
            logger.logWarn("Header too large");
            return -2;
        }

        if (contentLength > 1000000) {
            logger.logWarn("Body too large");
            return -2;
        }

    }

   
}

void Server::getIpStr(const struct sockaddr *sa, char *s, size_t maxlen){
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            break;
    }
}

void Server::listen(int port){
    //Listening for connections on the socket
    int serverSocketFd = createServerSocket(port);
    ::listen(serverSocketFd, BACKLOG);
    logger.logInfo("Server listening on port " + to_string(port));

    //listening and handling incoming requests
    listenRequests(serverSocketFd);
}

int Server::createServerSocket(int port){
    //setting up the address struct, responsible for server address and port
    struct addrinfo address, *addressList;
    memset(&address, 0, sizeof address);

    address.ai_family = AF_UNSPEC;
    address.ai_socktype = SOCK_STREAM;
    address.ai_flags = AI_PASSIVE;

    const char *port_str = to_string(port).c_str();
    int status = getaddrinfo(NULL, port_str, &address, &addressList);
    if (status != 0) {
        logger.logError("Getaddrinfo error: " + string(gai_strerror(status)));
        exit(1);
    }

    //Fetching the first valid address and binding to it
    struct sockaddr *addr = NULL;
    int serverSoc = -1;
    for(struct addrinfo *p = addressList; p!=NULL; p = p->ai_next){
        //Creating server side socket
        if(serverSoc = socket(p->ai_family, p->ai_socktype, p->ai_protocol); serverSoc == -1){
            logger.logDebug("Socket error: " + string(strerror(errno)));
            continue;
        }
        int val = 1;
        if(setsockopt(serverSoc, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (int)) == -1){
            close (serverSoc);
            continue;
        }
        //Binding server side socket to the address
        if(bind(serverSoc, p->ai_addr,p->ai_addrlen) == -1){
            close (serverSoc);
            logger.logError("Bind error: " + string(strerror(errno)));
            continue;
        }else{
            addr = p->ai_addr;
            break;
        }
    }

    //No valid address found
    if(addr == NULL){
        logger.logError("Failed to bind to any address");
        exit(1);
    }
    freeaddrinfo(addressList); //Freeing the address list allocated by getaddrinfo
    return serverSoc;
}

void Server::use(middelwareFunction func){
    middelwareFunctions.push_back(func);
}

void Server::route(const string &method, const string &path, handlerFunction &handler){
    if(find(SUPPORTED_METHODS.begin(), SUPPORTED_METHODS.end(), method) == SUPPORTED_METHODS.end()){
        logger.logError("Attempted to register unsupported HTTP method: " + method);
        throw NotSupportedException("HTTP Method " + method + " is not supported");
    }
    
    string routeKey = path;
    routeHandlers.push_back(RouteHandler(method, path, handler)); 
}

handlerFunction Server::getRouteHandlerForPath(Request &request){
    //We basically first match the path and then check if the method matches, we will pick the first match and return
    string method = request.method;
    string path = request.path;
    for(auto &routeHandler: routeHandlers){
        unordered_map<string, string> params;
        bool isPathMatched = validatePath(routeHandler.path, path, params);
        if(isPathMatched && routeHandler.method == method){
            request.params = params;
            return routeHandler.handler;
        }
    }
    //return empty function if no match found
    return handlerFunction(); 
}

//the params in a dynamic route will defined as :paramName. Ex: /user/:id/profile
bool Server::validatePath(string routePath, string requestPath, unordered_map<string, string> &params){
    vector<string> routeSegments;
    vector<string> requestSegments;

    stringstream routeSS(routePath);
    string segment;
    while(getline(routeSS, segment, '/')){
        if(!segment.empty()){
            routeSegments.push_back(segment);
        }
    }

    stringstream requestSS(requestPath);
    while(getline(requestSS, segment, '/')){
        if(!segment.empty()){
            requestSegments.push_back(segment);
        }
    }

    if(routeSegments.size() != requestSegments.size()){
        return false;
    }

    for(size_t i=0; i<routeSegments.size(); i++){
        if(routeSegments[i].front() == ':' && routeSegments[i].length() > 1){
            //parameter segment
            string paramName = routeSegments[i].substr(1);
            params[paramName] = requestSegments[i];
        }else if(routeSegments[i] != requestSegments[i]){
            //no match
            params.clear();
            return false;
        }
    }

    return true;
}

void Server::get(const string &path, handlerFunction handler){
    route("GET", path, handler);
}

void Server::post(const string &path, handlerFunction handler){
    route("POST", path, handler);
}

void Server::put(const string &path, handlerFunction handler){
    route("PUT", path, handler);
}

void Server::del(const string &path, handlerFunction handler){
    route("DELETE", path, handler);
}