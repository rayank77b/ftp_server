#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

constexpr int PORT = 2121;    // Use 2121 for testing (21 needs root)
constexpr int BACKLOG = 5;

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // 2. Bind to address
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Bind failed\n";
        close(server_fd);
        return 1;
    }

    // 3. Listen for connections
    if (listen(server_fd, BACKLOG) == -1) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }

    std::cout << "FTP server listening on port " << PORT << "...\n";

    // 4. Accept a client connection
    client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
    if (client_fd == -1) {
        std::cerr << "Accept failed\n";
        close(server_fd);
        return 1;
    }

    // 5. Send welcome message
    const char* welcome = "220 Simple FTP Server Ready\r\n";
    send(client_fd, welcome, strlen(welcome), 0);
    std::cout << "Client connected. Welcome message sent.\n";

    // 6. Close sockets
    close(client_fd);
    close(server_fd);

    return 0;
}
