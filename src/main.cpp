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
using namespace std;


int main(){
    //setting up the address struct, responsible for server address and port
    struct addrinfo address, *addressList;
    memset(&address, 0, sizeof address);
    address.ai_family = AF_UNSPEC;
    address.ai_socktype = SOCK_STREAM;
    address.ai_flags = AI_PASSIVE;
    const char *port = "8888";
    int status = getaddrinfo(NULL, port, &address, &addressList);
    if (status != 0) {
       cerr <<"getaddrinfo error\n" << gai_strerror(status)<<endl;
        return 1;
    }

    //Fetching the first valid address and binding to it
    struct sockaddr *addr = NULL;
    int serverSoc = -1;
    for(struct addrinfo *p = addressList; p!=NULL; p = p->ai_next){
        //Creating server side socket
        if(serverSoc = socket(p->ai_family, p->ai_socktype, p->ai_protocol); serverSoc == -1){
            cerr << "socket error: " << strerror(errno) << endl;
            continue;
        }
        int val = 1;
        if(setsockopt(serverSoc, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (int)) == -1){
            cerr << "setsockopt error: " << strerror(errno) << endl;
            close (serverSoc);
            continue;
        }
        //Binding server side socket to the address
        if(bind(serverSoc, p->ai_addr,p->ai_addrlen) == -1){
            close (serverSoc);
            cerr << "bind error: " << strerror(errno) << endl;
            continue;
        }else{
            addr = p->ai_addr;
            break;
        }
    }

    //No valid address found
    if(addr == NULL){
        cerr << "failed to bind" << endl;
        return 1;
    }

    //Listening for connections on the socket
    listen(serverSoc, 10);
    cout << "server is listening..." << endl;

    while(true){
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSoc = accept(serverSoc, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if(clientSoc == -1){
            continue;
        }

        //Handle new client connection
        struct sockaddr *clientAddrPtr = (struct sockaddr *)&clientAddr;
        bool isIPv4 = clientAddrPtr->sa_family== AF_INET;
        int addressLen = isIPv4? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
        char ip[addressLen];

        inet_ntop(clientAddr.ss_family, isIPv4? (void*)&(((struct sockaddr_in *)clientAddrPtr)->sin_addr) : (void *)&(((struct sockaddr_in6 *)clientAddrPtr)->sin6_addr), ip, addressLen);
        cout<<"Accepted a new client connection" << endl;
        cout<<"Ip address: "<<ip<<endl;
        send(clientSoc, "Hello, World!\n", 14, 0);
        close(clientSoc);
    }
    freeaddrinfo(addressList);
}



// struct addrinfo {
//     int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
//     int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
//     int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
//     int              ai_protocol;  // use 0 for "any"
//     size_t           ai_addrlen;   // size of ai_addr in bytes
//     struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
//     char            *ai_canonname; // full canonical hostname

//     struct addrinfo *ai_next;      // linked list, next node
// };

// struct sockaddr {
//     unsigned short    sa_family;    // address family, AF_xxx
//     char              sa_data[14];  // 14 bytes of protocol address
// }; 

// struct sockaddr_in {
//     short int          sin_family;  // Address family, AF_INET
//     unsigned short int sin_port;    // Port number
//     struct in_addr     sin_addr;    // Internet address
//     unsigned char      sin_zero[8]; // Same size as struct sockaddr
// };

// // Internet address (a structure for historical reasons)
// struct in_addr {
//     uint32_t s_addr; // that's a 32-bit int (4 bytes)
// };

// struct sockaddr_in6 {
//     u_int16_t       sin6_family;   // address family, AF_INET6
//     u_int16_t       sin6_port;     // port, Network Byte Order
//     u_int32_t       sin6_flowinfo; // IPv6 flow information
//     struct in6_addr sin6_addr;     // IPv6 address
//     u_int32_t       sin6_scope_id; // Scope ID
// };

// struct in6_addr {
//     unsigned char   s6_addr[16];   // IPv6 address
// };
