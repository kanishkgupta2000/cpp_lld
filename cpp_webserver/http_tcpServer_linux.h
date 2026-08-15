#ifndef INCLUDED_HTTP_TCPSERVER_LINUX
#define INCLUDED_HTTP_TCPSERVER_LINUX

/* Learnings: (google style convention)
What is #ifndef #define and #endif purpose?
These are called #include guards. 
Once the header is included, it checks if a unique value is defined. 
Then if it's not defined, it defines it and continues to the rest of the page. 

When the code is included again, the first ifndef fails, resulting in a blank file.
Prevents double declaration of any identifiers such as types, enums and static variables.
*/

#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string>

namespace http
{
    class TcpServer{
        public:
        TcpServer(std::string ip_address, int port);
        ~TcpServer();
        void startListen();

        // Learning: m_ is just a naming convention for naming member variables to avoid using this-> operators in case of ambiguity between ctor params and member variables
        private:
        std::string m_ip_address;
        int m_port;
        int m_socket;
        int m_new_socket;
        long m_incomingMessage;
        struct sockaddr_in m_socketAddress;
        unsigned int m_socketAddress_len;
        std::string m_serverMessage;
        
        int startServer();
        void closeServer();
        void acceptConnection(int &new_socket);
        std::string buildResponse(std::string &response);
        void sendResponse(std::string &response);
    };
} // namespace http
#endif
