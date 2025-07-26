#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <sstream>

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
    DataConn dataconn;

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
        } else if (cmd == "PASV") {
            if (openPassiveDataConn(dataconn, client_fd)) {
                // Send PASV reply (RFC 959 format: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2))
                // Use local IP, e.g., 127,0,0,1
                int p1 = dataconn.port / 256;
                int p2 = dataconn.port % 256;
                std::string reply = "227 Entering Passive Mode (127,0,0,1," +
                    std::to_string(p1) + "," + std::to_string(p2) + ")\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
            } else {
                std::string reply = "425 Can't open data connection\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
            }
        } else if (cmd == "LIST") {
            if (!dataconn.ready) {
                std::string reply = "425 Use PASV first\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                continue;
            }
            int data_fd = acceptPassiveDataConn(dataconn);
            if (data_fd == -1) {
                std::string reply = "425 Data connection failed\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                closeDataConn(dataconn);
                continue;
            }
            std::string reply = "150 Here comes the directory listing\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);

            // Run 'ls -l', send output
            FILE* ls = popen("ls -l", "r");
            if (ls) {
                char lbuf[1024];
                while (fgets(lbuf, sizeof(lbuf), ls)) {
                    send(data_fd, lbuf, strlen(lbuf), 0);
                }
                pclose(ls);
            }
            close(data_fd);
            closeDataConn(dataconn);
            dataconn.ready = false;
            reply = "226 Directory send OK\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        } else if (cmd == "NOOP") {
            std::string reply = "200 NOOP ok\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        } else {
            std::string reply = "502 Command not implemented\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        }
    }
    // Close client socket
    close(client_fd);
    closeDataConn(dataconn);
    Logger::log(Logger::INFO, "Client disconnected.");
}

bool FtpServer::openPassiveDataConn(DataConn& dataconn, int /*control_fd*/) {
    dataconn.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (dataconn.listen_fd == -1) return false;

    sockaddr_in data_addr{};
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Try random port between 20000-21000
    for (int p = 20000; p <= 21000; ++p) {
        data_addr.sin_port = htons(p);
        if (bind(dataconn.listen_fd, (sockaddr*)&data_addr, sizeof(data_addr)) == 0) {
            dataconn.port = p;
            break;
        }
    }
    if (dataconn.port == 0) {
        close(dataconn.listen_fd);
        dataconn.listen_fd = -1;
        return false;
    }
    if (listen(dataconn.listen_fd, 1) != 0) {
        close(dataconn.listen_fd);
        dataconn.listen_fd = -1;
        dataconn.port = 0;
        return false;
    }
    dataconn.ready = true;
    return true;
}

int FtpServer::acceptPassiveDataConn(DataConn& dataconn) {
    sockaddr_in cli_addr{};
    socklen_t clen = sizeof(cli_addr);
    dataconn.conn_fd = accept(dataconn.listen_fd, (sockaddr*)&cli_addr, &clen);
    close(dataconn.listen_fd);
    dataconn.listen_fd = -1;
    return dataconn.conn_fd;
}

void FtpServer::closeDataConn(DataConn& dataconn) {
    if (dataconn.conn_fd != -1) close(dataconn.conn_fd);
    if (dataconn.listen_fd != -1) close(dataconn.listen_fd);
    dataconn.conn_fd = -1;
    dataconn.listen_fd = -1;
    dataconn.ready = false;
    dataconn.port = 0;
}
