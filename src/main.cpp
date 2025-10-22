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
#include<server.h>
#include<middleware.h>
#include<routeHandler.h>
#include<request.h>
#include<response.h>
using namespace std;

middelwareFunction sampleMiddlewareFunc = [](Request &req, Response &res)->void{
    cout<<"Middleware function in action"<<endl;
    cout<<"Do some preprocessing before actual handler will run"<<endl;
};

handlerFunction sampleHandlerFunc = [](Request &req, Response &res)->void{
    cout<<"Printing the headers"<<endl;
    for(auto header: req.headers){
        cout<<"Header req["<<header.first<<"]"<<"="<<header.second<<endl;
    }

    cout<<"Printing the query params"<<endl;
    for(auto qParam: req.queryParams){
        cout<<"Query param: "<<qParam.first<<"="<<qParam.second<<endl;
    }

    cout<<"Printing the dynamic path params"<< endl;
     for(auto param: req.params){
        cout<<"path param: "<<param.first<<"="<<param.second<<endl;
    }

    cout<<"Printing the body of the request"<<endl;
    cout<<req.body<<endl;


    //The use has access to all the required data in this handler, everything is abstracted away
    //The user can simply focus on writing core logic


    cout<<"Sending the response"<<endl;
    res.setStatus(200);
    res.setBody("Everything is working fine");
    res.sendResponse();
};


int main(){
    int port = 8888;
    Server server;

    // Register middleware here
    server.use(sampleMiddlewareFunc);
    
    // Register the methods and handlers here
    server.get("/test/:testId/what/:whatId", sampleHandlerFunc);
    //server.POST("/submit", submitHandler);
    //server.PUT("/update", updateHandler);
    //server.DELETE("/delete", deleteHandler);

    // Start listening on the specified port
    server.listen(port);
    return 0;    
}


//To do
// 1. Do request parsing, create functions for handling different HTTP methods (GET, POST, etc.).
// 2. Construct appropriate HTTP responses based on the requests.
// 3. Implement concurrency to handle multiple client connections simultaneously.
// 4. Handle params and query params in header file. 
// 5. Add support to send static files (HTML, CSS, JS, images) from the server to the client.
// 6. Implement routing mechanism to map URLs to specific handler functions.
// 7. Persistent connections (Keep-Alive).
// 8. Add a middleware system to process requests and responses.  


