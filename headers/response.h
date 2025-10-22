#pragma once
#include<string>
#include<unordered_map>
#include<logger.h>

class Response{
    public:
        std::string httpVersion;
        int statusCode;
        std::string statusMessage;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
        int sendResponseStatus;
        int socketFd;
        Response(int socketFd);
        void setHeader(const std::string &key, const std::string &value);
        int sendResponse();
        void setStatus(int code);
        void setBody(const std::string &bodyContent);
        void setMessage(const std::string &message);
        std::string getMessageForStatusCode(int code);
};