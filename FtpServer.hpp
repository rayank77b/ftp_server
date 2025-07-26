#pragma once

class FtpServer {
public:
    FtpServer(int port = 2121);
    ~FtpServer();
    void run();

private:
    int port_;
    int server_fd_;
};
