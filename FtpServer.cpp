#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> 

#include "FtpServer.hpp"
#include "Logger.hpp"
#include "ErrorHandler.hpp"
#include "CommandParser.hpp"

FtpServer::FtpServer(int port)
    : port_(port), server_fd_(-1) {

    addr = {};
    addrlen = sizeof(addr);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
}

FtpServer::~FtpServer() {
    if (server_fd_ != -1) {
        close(server_fd_);
    }
}

void FtpServer::run() {
    // 1. Create socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        ErrorHandler::handleError("Failed to create socket", true);
    }

    Logger::log(Logger::INFO, "Socket created.");

    // 2. Bind to address
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(server_fd_);
        server_fd_ = -1;
        ErrorHandler::handleError("Bind failed", true);
    }
    Logger::log(Logger::INFO, "Bind successful.");

    // 3. Listen for connections
    if (listen(server_fd_, 5) == -1) {
        std::cerr << "Listen failed\n";
        close(server_fd_);
        server_fd_ = -1;
        ErrorHandler::handleError("Listen failed", true);
    }
    Logger::log(Logger::INFO, "FTP server Listening on port " + std::to_string(port_) + "...");

    // 4. Accept a client connection
    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd_,  (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        ErrorHandler::handleError("Accept failed");
        return;
    }
    // Log client IP and port
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);
    Logger::log(Logger::INFO, "Client connected: " + client_ip + ":" + std::to_string(client_port));

    // 5. Handle Session
    handleSession(client_fd, client_ip, client_port);
   
}

void FtpServer::handleSession(int client_fd, const std::string& client_ip, int client_port) {
    // Send welcome message
    const char* welcome = "220 Simple FTP Server Ready\r\n";
    if(send(client_fd, welcome, strlen(welcome), 0)==-1)
        ErrorHandler::handleError("Failed to send welcome message");
    else
        Logger::log(Logger::INFO, "Welcome message sent.");

    char buf[4096];
    bool running = true;
    bool logged_in = false;
    std::string last_user;

    while (running) {
        ssize_t received = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (received <= 0) {
            Logger::log(Logger::INFO, "Connection closed by client or error occurred.");
            break;
        }
        buf[received] = '\0';
        std::string input(buf);

        Logger::log(Logger::INFO, "Received: " + input);

        // Parse command and argument
        auto [cmd, arg] = CommandParser::parse(input);

        if (cmd == "USER") {
            last_user = arg;
            std::string reply = "331 Username ok, need password\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        } else if (cmd == "PASS") {
            // Accept any password
            logged_in = true;
            std::string reply = "230 User logged in, proceed\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        } else if (cmd == "NOOP") {
            std::string reply = "200 NOOP ok\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        } else if (cmd == "QUIT") {
            std::string reply = "221 Goodbye\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
            running = false;
        } else {
            std::string reply = "502 Command not implemented\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        }
    }
    // Close client socket
    close(client_fd);
    Logger::log(Logger::INFO, "Client disconnected.");
}