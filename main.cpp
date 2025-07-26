#include "FtpServer.hpp"
#include "Logger.hpp"

int main() {

    Logger::init("ftpserver.log"); // or "" for console only

    FtpServer server(2121);
    server.run();

    Logger::close();
    
    return 0;
}