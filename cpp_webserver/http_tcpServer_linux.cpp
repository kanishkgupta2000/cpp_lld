#include "http_tcpServer_linux.h"
#include "rest_pipeline.h"
#include "server_config.h"
#include<iostream>
#include<sstream>
#include<unistd.h>
#include <spdlog/spdlog.h>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

/*
Recap :: is the scope resolution operator
The header file contains the declaration of the constructor and destructor. 
This file will contain the definition of the constructor and destructor.

ex:

TcpServer::TcpServer
1st TcpServer means the class
2nd TcpServer means the constructor
Since both header file and this file maintain the same namespace http, we need not prefix the class with scope http
Else the scope resolution would look like http::TcpServer::TcpServer()
*/

namespace {
    /*
    This is called an unnamed (anonymous namespace).
    What does it mean?
    "These functions are private to this .cpp file"
    */
    boost::asio::thread_pool pool(16);

    const int BUFFER_SIZE = 30720;
    const bool silentMode = true;
    void exitWithError(const std::string &errorMessage)
    {
        spdlog::error("ERROR: " + errorMessage);
        exit(1);
    }
}

namespace http{
    TcpServer::TcpServer(const ServerConfig &config): m_ip_address(config.host), m_port(config.port),
     m_run_sequential(config.run_sequential), m_socket(), m_new_socket(), m_incomingMessage(),
     m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)), m_serverMessage()
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());
        if (startServer() != 0)
        {
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            spdlog::critical(ss.str());
        }
    }
    
    TcpServer::~TcpServer()
    {
        closeServer();
    }

    int TcpServer::startServer()
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            exitWithError("Cannot create socket");
            return 1;
        }

        if (bind(m_socket,(sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::closeServer()
    {
        close(m_socket);
        close(m_new_socket);
        exit(0);
    }

    void TcpServer::startListen()
    {
        if (listen(m_socket, 20) < 0)
        {
            exitWithError("Socket listen failed");
        }
        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        spdlog::info(ss.str());

        while(true)
        {
            spdlog::info("=====Waiting for a new connection ===== \n\n");
            acceptConnection(m_new_socket);
            int client_socket = m_new_socket;

            if (m_run_sequential)
            {
                process_request(client_socket);
            }
            else
            {
                boost::asio::post(pool, [this, client_socket]()
                              { 
                                process_request(client_socket);
                             });
            }
            
        }
    }

    void TcpServer::process_request(int client_socket)
    {
        char buffer[BUFFER_SIZE] = {0};
        int bytesReceived = read(client_socket, buffer, BUFFER_SIZE);
        if (bytesReceived < 0)
        {
            exitWithError("Failed to read bytes from client socket connection");
        }

        std::ostringstream ss;

            ss << "------ Received Request from client ------\n\n";
            spdlog::info(ss.str());
        std::string content(buffer);
        spdlog::info("buffer is: \n" + content);

        Rest::RestExecutor executor(content);
        std::string response = executor.ExecuteQuery();
        spdlog::info (response);
        std::string builtResponse = buildResponse(response);

        sendResponse(builtResponse, client_socket);

        close(client_socket);
    }

    void TcpServer::acceptConnection(int &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, 
                            &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss << 
            "Server failed to accept incoming connection from ADDRESS: " 
            << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " 
            << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse(std::string &response)
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :) </p> <p>" + response + "</p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

    void TcpServer::sendResponse(std::string &message, int socket)
    {
        long bytesSent;

        bytesSent = write(socket, message.c_str(), message.size());

        if (bytesSent == message.size())
        {
            spdlog::info("------ Server Response sent to client ------\n\n");
        }
        else
        {
            spdlog::error("Error sending response to client");
        }
    }
} // namespace http
