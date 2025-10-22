#pragma once
#include<string>
#include<logger.h>
#include<unordered_map>
#include<routeHandler.h>
#include<middleware.h>

const std::string LOG_FILE = "logs/server.log";

//Main server class definition
class Server{
    private:
        Logger logger;
        std::vector<RouteHandler> routeHandlers;
        std::vector<middelwareFunction> middelwareFunctions;
        int createServerSocket(int port);
        void listenRequests(int serverSocket);
        void getIpStr(const struct sockaddr *sa, char *s, size_t maxlen);
        int receiveHttpRequest(int clientSocketFd, std::string &requestHeaders,std::string &requestBody);
        handlerFunction getRouteHandlerForPath(Request &request);
        bool validatePath(std::string routePath, std::string requestPath,std::unordered_map<std::string, std::string> &params);
    public:
        Server();
        void listen(int port);
        void route(const std::string &method, const std::string &path, handlerFunction &handler);
        // void use(middelwareFunction middleware);
        void get(const std::string &path, handlerFunction handler);
        void post(const std::string &path, handlerFunction handler);
        void put(const std::string &path, handlerFunction handler);
        void del(const std::string &path, handlerFunction handler);
        void use(middelwareFunction);
};

