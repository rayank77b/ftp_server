#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <thread>

#include "UserAuth.hpp"

class FtpServer {
public:
    FtpServer(int port = 2121, const std::string& root_dir = "./ftp_root");
    ~FtpServer();
    void run();

private:
    int port_;
    int server_fd_;
    std::string root_dir_;
    UserAuth userauth_;

    struct DataConn {
        int listen_fd = -1;
        int conn_fd = -1;
        int port = 0;
        bool ready = false;
    };

    struct sockaddr_in addr;
    socklen_t addrlen;

    void handleSession(int client_fd, const std::string& client_ip, int client_port);

    // Helpers for data connection
    bool openPassiveDataConn(DataConn& dataconn, int control_fd);
    int  acceptPassiveDataConn(DataConn& dataconn);
    void closeDataConn(DataConn& dataconn);
    std::string resolvePath(const std::string& root, const std::string& user_path);
    bool isPathAllowed(const std::string& root, const std::string& user_path);
};
