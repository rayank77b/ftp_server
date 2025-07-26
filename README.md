
What Does an FTP Server Do?

    Listens for incoming connections on the FTP port (default: 21).

    Authenticates clients (username/password, or anonymous).

    Parses and responds to FTP commands (USER, PASS, LIST, RETR, STOR, QUIT, etc).

    Manages file transfers—sending and receiving files.

    Handles passive/active modes for data transfer (for basic version, pick one).

    Manages sessions (each client is a session with state).


Project Plan & Steps
1. Preparation

    Understand the FTP protocol basics.

    Decide on basic functionality to implement:

        User authentication (simple, or allow anonymous only?)

        Commands: USER, PASS, LIST, RETR (download), STOR (upload), QUIT.

2. Skeleton Structure

    main.cpp: Sets up and runs the server loop.

    FtpServer class: Handles listening and accepting clients.

    FtpSession class: Handles communication with one client (per connection/thread/process).

    Helper classes: For parsing commands, file I/O, etc.

3. Network Setup

    Open a listening TCP socket on port 21 (or >1024 if you don’t run as root).

    Accept incoming connections in a loop.

    For each new connection, spawn a new session (thread/process/fork for multi-client, or just handle one for first version).

4. Session Handling

    For each client connection:

        Send a welcome message (220 Service ready).

        Enter command loop:

            Read a line from the client.

            Parse FTP command and arguments.

            Respond with the appropriate reply code/message.

        Close connection on QUIT or error.

5. Command Parsing and Handling

    Implement command parsing (split by space, uppercase).

    For a minimal server, support:

        USER <username>

        PASS <password>

        LIST (list files in current directory)

        RETR <filename> (send file to client)

        STOR <filename> (receive file from client)

        QUIT

    For data transfer, use passive mode (PASV) only for simplicity.

6. File System Operations

    Map client commands to actual file system operations.

    Make sure to handle errors securely (don’t allow .. path traversal etc).

7. Data Connection (Passive Mode, PASV)

    When client sends a command requiring data transfer (LIST, RETR, STOR):

        Server opens a new data socket (random port), sends port info to client.

        Client connects to that port for data transfer.

    Only implement PASV, not PORT/active mode (much easier for a toy server).

8. Clean Shutdown & Logging

    Handle client disconnects and errors cleanly.

    Optionally log actions (stdout, or simple file).

9. Testing

    Test using a standard FTP client (e.g. ftp, lftp, FileZilla).

    Use a non-privileged port for development (2121 etc).