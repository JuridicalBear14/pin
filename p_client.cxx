#include "client.hxx"

/* Pass control to interface class */
void* start_interface(void* args) {
    Client* c = (Client*) args;
    c->interface_handler();

    return NULL;
}

/* Pass control to client class */
void* start_listener(void* args) {
    Client* c = (Client*) args;
    c->recieve();

    return NULL;
}


int main(int argc, char** argv) {
    // Set SIGPIPE to ignore, that way we don't crash froma broken connection
    signal(SIGPIPE, SIG_IGN);

    int client_fd;
    std::string ip = "127.0.0.1";
    std::string name = "";
    std::string key = "";

    // Name given
    if (argc > 2) {
        name = argv[2];
        if (name.length() > NAMELEN) {  // Clip off name
            name = name.substr(0, NAMELEN);
        }
    }

    // Key given
    if (argc > 3) {
        key = argv[3];
    }

    // ip given
    if (argc > 1 && (strcmp(argv[1], "local"))) {
        ip = argv[1];
    }

    int status, valread;
    struct sockaddr_in serv_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(DEFAULT_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    // Before connecting, get client login info
    Client c;
    c.user_login(name, key);

    // Connect to server
    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        std::cerr << "\nConnection Failed \n";
        return -1;
    }

    // Now that we connected, set timeout for sending and recieving
    /*struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv)) || setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*) &tv, sizeof(tv))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }*/
    
    // Now initialize connection
    if (c.init(client_fd) != E_NONE) {
        close(client_fd);
        return -1;
    }

    std::thread listener_t(start_listener, &c);
    std::thread interface_t(start_interface, &c);

    // Cleanup threads
    listener_t.detach();    // Detach this guy, since we only want to wait on GUI
    interface_t.join();

    // Closing the connected socket
    close(client_fd);
    return 0;
}
