#include "FtpServer.hpp"

int main() {
    FtpServer server(2121);
    server.run();
    return 0;
}