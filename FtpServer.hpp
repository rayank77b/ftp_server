#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class FtpServer {
public:
    FtpServer(int port = 2121);
    ~FtpServer();
    void run();

private:
    int port_;
    int server_fd_;

    struct sockaddr_in addr;
    socklen_t addrlen;
};
