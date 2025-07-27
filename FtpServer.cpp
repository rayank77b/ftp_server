#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <sstream>
#include <libgen.h> // dirname
#include <cstring>  // for strdup

#include "FtpServer.hpp"
#include "Logger.hpp"
#include "ErrorHandler.hpp"
#include "CommandParser.hpp"

FtpServer::FtpServer(int port, const std::string& root_dir)
    : port_(port), server_fd_(-1), root_dir_(root_dir) {

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

    // load userfile
    if (!userauth_.loadFromFile("users.txt")) {
        Logger::log(Logger::ERROR, "Failed to load user file users.txt");
        return;
    }

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

    // 4. Accept connections in a loop (multi-client)
    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            Logger::log(Logger::ERROR, "Accept failed");
            continue;
        }
        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = ntohs(client_addr.sin_port);
        Logger::log(Logger::INFO, "Client connected: " + client_ip + ":" + std::to_string(client_port));

        // Launch a thread for each client
        std::thread([this, client_fd, client_ip, client_port]() {
            handleSession(client_fd, client_ip, client_port);
            Logger::log(Logger::INFO, "Client disconnected: " + client_ip + ":" + std::to_string(client_port));
        }).detach();
    }
   
}

void FtpServer::handleSession(int client_fd, const std::string& client_ip, int client_port) {
    // change to ftp_root
    //chdir(root_dir_.c_str());

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
            if (userauth_.checkPassword(last_user, "")) {
                std::string reply = "230 User logged in, proceed\r\n"; // In case of empty password allowed
                logged_in = true;
                send(client_fd, reply.c_str(), reply.length(), 0);
            } else {
                std::string reply = "331 Username ok, need password\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
            }
        } else if (cmd == "PASS") {
            if (last_user.empty()) {
                std::string reply = "503 Login with USER first\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
            } else if (userauth_.checkPassword(last_user, arg)) {
                logged_in = true;
                std::string reply = "230 User logged in, proceed\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
            } else {
                std::string reply = "530 Login incorrect\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                last_user.clear();
            }
        } else if (cmd == "NOOP") {
            std::string reply = "200 NOOP ok\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
        } else if (cmd == "QUIT") {
            std::string reply = "221 Goodbye\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
            running = false;
        } else if (!logged_in && cmd != "QUIT" && cmd != "NOOP") {
            std::string reply = "530 Please login with USER and PASS\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);
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

            // Run 'ls -l path', send output
            std::string listdir = resolvePath(root_dir_, arg.empty() ? "." : arg);
            Logger::log(Logger::INFO, "listdir: "+listdir);

            if (listdir.empty()) {
                std::string reply = "550 Invalid directory\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                close(data_fd);
                closeDataConn(dataconn);
                continue;
            }
            std::string cmd = "ls -l " + listdir;
            FILE* ls = popen(cmd.c_str(), "r");
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
        } else if (cmd == "RETR") {
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
            std::string filepath = resolvePath(root_dir_, arg);
            if (filepath.empty()) {
                std::string reply = "550 File not found\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                close(data_fd);
                closeDataConn(dataconn);
                continue;
            }
            FILE* f = fopen(filepath.c_str(), "rb");
            if (!f) {
                std::string reply = "550 File not found\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                close(data_fd);
                closeDataConn(dataconn);
                continue;
            }
            std::string reply = "150 Opening data connection for file transfer\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);

            char filebuf[4096];
            size_t n;
            while ((n = fread(filebuf, 1, sizeof(filebuf), f)) > 0) {
                if (send(data_fd, filebuf, n, 0) < 0) break;
            }
            fclose(f);
            close(data_fd);
            closeDataConn(dataconn);
            dataconn.ready = false;
            reply = "226 Transfer complete\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);

        } else if (cmd == "STOR") {
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
            if (!isPathAllowed(root_dir_, arg)) {
                std::string reply = "550 Invalid path\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                close(data_fd);
                closeDataConn(dataconn);
                continue;
            }
            std::string filepath = root_dir_ + "/" + arg;
            FILE* f = fopen(filepath.c_str(), "wb");
            if (!f) {
                std::string reply = "550 Cannot open file for writing\r\n";
                send(client_fd, reply.c_str(), reply.length(), 0);
                close(data_fd);
                closeDataConn(dataconn);
                continue;
            }
            std::string reply = "150 Ok to send data\r\n";
            send(client_fd, reply.c_str(), reply.length(), 0);

            char filebuf[4096];
            ssize_t n;
            while ((n = recv(data_fd, filebuf, sizeof(filebuf), 0)) > 0) {
                fwrite(filebuf, 1, n, f);
            }
            fclose(f);
            close(data_fd);
            closeDataConn(dataconn);
            dataconn.ready = false;
            reply = "226 Transfer complete\r\n";
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

// Returns the real absolute path, or "" on error
std::string FtpServer::resolvePath(const std::string& root, const std::string& user_path) {
    std::string candidate = root + "/" + user_path;

    char absbuf[PATH_MAX];
    if (realpath(candidate.c_str(), absbuf) == nullptr) return "";

    std::string abs(absbuf);
    // Ensure abs starts with root's real path
    char rootbuf[PATH_MAX];
    if (realpath(root.c_str(), rootbuf) == nullptr) return "";
    
    std::string absroot(rootbuf);
    if (abs.compare(0, absroot.size(), absroot) != 0) return ""; // Outside jail!
    
    return abs;
}

bool FtpServer::isPathAllowed(const std::string& root, const std::string& user_path) {
    // Find parent directory
    std::string candidate = root + "/" + user_path;
    char* cand_dup = strdup(candidate.c_str()); // dirname may modify the input
    std::string parent_dir = dirname(cand_dup);
    free(cand_dup);

    char parent_abs[PATH_MAX];
    if (realpath(parent_dir.c_str(), parent_abs) == nullptr)
        return false; // Parent does not exist
    char root_abs[PATH_MAX];
    if (realpath(root.c_str(), root_abs) == nullptr)
        return false;
    // Must be inside jail
    std::string abs_parent(parent_abs);
    std::string abs_root(root_abs);
    return abs_parent.compare(0, abs_root.size(), abs_root) == 0;
}