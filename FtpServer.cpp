#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "FtpServer.hpp"


FtpServer::FtpServer(int port)
    : port_(port), server_fd_(-1) {}

FtpServer::~FtpServer() {
    if (server_fd_ != -1) {
        close(server_fd_);
    }
}

void FtpServer::run() {
    struct sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);

    // 1. Create socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    // 2. Bind to address
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Bind failed\n";
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    // 3. Listen for connections
    if (listen(server_fd_, 5) == -1) {
        std::cerr << "Listen failed\n";
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    std::cout << "FTP server listening on port " << port_ << "...\n";

    // 4. Accept a client connection
    int client_fd = accept(server_fd_, (struct sockaddr*)&addr, &addrlen);
    if (client_fd == -1) {
        std::cerr << "Accept failed\n";
        return;
    }

    // 5. Send welcome message
    const char* welcome = "220 Simple FTP Server Ready\r\n";
    send(client_fd, welcome, strlen(welcome), 0);
    std::cout << "Client connected. Welcome message sent.\n";

    // 6. Close client socket
    close(client_fd);
}
