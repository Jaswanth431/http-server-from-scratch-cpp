#ifndef SERVER_H
#define SERVER_H
#include<string>
#include<logger.h>
using namespace std;

const string LOG_FILE = "logs/server.log";

//Main server class definition
class Server{
    private:
        Logger logger;
        int createServerSocket(int port);
        void listenRequests(int serverSocket);
    public:
        Server();
        void listen(int port);
};

#endif // SERVER_H
