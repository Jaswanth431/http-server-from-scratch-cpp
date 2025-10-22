#include<response.h>
#include<string.h>
#include<sys/socket.h>
#include<logger.h>
#include<unistd.h>
using namespace std;

Response::Response(int socketFileDescr): socketFd(socketFileDescr){
    httpVersion = "HTTP/1.1";
    statusCode = 200;
    statusMessage = "OK";
    body = "";
    //Default headers
    setHeader("Connection", "close");
    setHeader("Server", "mini-nginx/1.0");
}

string Response::getMessageForStatusCode(int code){
    switch(code){
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 503:
            return "Service Unavailable";
        default:
            return "Unknown Status";
    }
}

void Response::setHeader(const std::string &key, const std::string &value){
    headers[key] = value;
}
int Response::sendResponse(){
    //construction of response string
    string response = httpVersion + " " + to_string(statusCode) + " " + statusMessage + "\r\n";
    for(auto &headerPair: headers){
        response += headerPair.first + ": " + headerPair.second + "\r\n";   
    }

    //End of headers
    response += "\r\n"; 
    if(!body.empty()){
        response += body;
    }
    
    //send response over socket
    size_t totalBytesSent = 0;
    size_t responseLength = response.length();
    while(totalBytesSent < responseLength){
        ssize_t bytesSent = send(socketFd, response.c_str() + totalBytesSent, responseLength - totalBytesSent, 0);
        if(bytesSent == 0){
            sendResponseStatus = 0;
            return 0;
        }else if(bytesSent < 0){
            if(errno == EINTR){
                //Interrupted by signal, try again
                continue;
            }
            sendResponseStatus = -1;
            return -1;
        }else{
            totalBytesSent += bytesSent;
        }
    }
    sendResponseStatus=1;
    return 1;
}
void Response::setStatus(int code){
    statusCode = code;
    statusMessage = getMessageForStatusCode(code);
}

void Response::setBody(const std::string &bodyContent){
    body = bodyContent;
    if(!body.empty()){
        setHeader("Content-Length", std::to_string(body.length()));
        setHeader("content-type", "text/plain");
    }
}

void Response::setMessage(const std::string &message){
    statusMessage = message;
}

