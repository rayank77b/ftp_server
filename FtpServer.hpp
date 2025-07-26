#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>

class FtpServer {
public:
    FtpServer(int port = 2121);
    ~FtpServer();
    void run();

private:
    void handleSession(int client_fd, const std::string& client_ip, int client_port);

    int port_;
    int server_fd_;

    struct sockaddr_in addr;
    socklen_t addrlen;
};
