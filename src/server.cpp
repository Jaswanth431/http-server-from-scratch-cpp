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
#include<server.h>

using namespace std;
const int BACKLOG = 10; //How many pending connections queue will hold

Server::Server():  logger(LOG_FILE, DEBUG) {
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
        bool isIPv4 = clientAddrPtr->sa_family== AF_INET;
        int addressLen = isIPv4? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
        char ip[addressLen];

        inet_ntop(clientAddr.ss_family, isIPv4? (void*)&(((struct sockaddr_in *)clientAddrPtr)->sin_addr) : (void *)&(((struct sockaddr_in6 *)clientAddrPtr)->sin6_addr), ip, addressLen);
        logger.logInfo("Accepted new connection from " + string(ip));
        send(clientSoc, "Hello, World!\n", 14, 0);
        close(clientSoc);
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
        logger.logError("Getaddrinfo error: " + std::string(gai_strerror(status)));
        exit(1);
    }

    //Fetching the first valid address and binding to it
    struct sockaddr *addr = NULL;
    int serverSoc = -1;
    for(struct addrinfo *p = addressList; p!=NULL; p = p->ai_next){
        //Creating server side socket
        if(serverSoc = socket(p->ai_family, p->ai_socktype, p->ai_protocol); serverSoc == -1){
            logger.logDebug("Socket error: " + std::string(strerror(errno)));
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
            logger.logError("Bind error: " + std::string(strerror(errno)));
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

