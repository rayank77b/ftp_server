#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <thread>



class FtpServer {
public:
    FtpServer(int port = 2121);
    ~FtpServer();
    void run();

private:
    struct DataConn {
        int listen_fd = -1;
        int conn_fd = -1;
        int port = 0;
        bool ready = false;
    };

    void handleSession(int client_fd, const std::string& client_ip, int client_port);

    // Helpers for data connection
    bool openPassiveDataConn(DataConn& dataconn, int control_fd);
    int  acceptPassiveDataConn(DataConn& dataconn);
    void closeDataConn(DataConn& dataconn);

    int port_;
    int server_fd_;

    struct sockaddr_in addr;
    socklen_t addrlen;
};
