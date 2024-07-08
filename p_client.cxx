#include "client.hxx"

/* Pass control to interface class */
void* start_interface(void* args) {
    Client* c = (Client*) args;
    c->start_interface();

    return NULL;
}

/* Pass control to client class */
void* start_listener(void* args) {
    Client* c = (Client*) args;
    c->recieve();

    return NULL;
}


int main(int argc, char** argv) {
    int client_fd;
    std::string ip = "127.0.0.1";
    std::string name = "NULL";

    // Name given
    if (argc > 1) {
        name = argv[1];
        if (name.length() > NAMELEN) {  // Clip off name
            name = name.substr(0, NAMELEN);
        }
    }

    // ip given
    if (argc > 2) {
        ip = argv[2];
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

    // Connect to server
    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        std::cerr << "\nConnection Failed \n";
        return -1;
    }
    
    Client c(name, client_fd);
    c.init();

    std::thread listener_t(start_listener, &c);
    std::thread interface_t(start_interface, &c);

    // Cleanup threads
    listener_t.detach();    // Detach this guy, since we only want to wait on GUI
    interface_t.join();

    // Closing the connected socket
    close(client_fd);
    return 0;
}