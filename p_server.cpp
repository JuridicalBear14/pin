#include "server.h"

void* start_server_relay(void* args) {
    Server* s = (Server*) args;
    s->start_server();

    return NULL;
}

// Main startup and port accept function
int main(void) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    std::cout << "Starting server...\n";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
       perror("setsockopt");
       exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 5555
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAXUSR) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    Server s(server_fd);

    // Set up relay thread
    std::thread r_thread(start_server_relay, &s);
    r_thread.detach();

    // Playing with printing IP
    /*
    char name[1024];
    gethostname(name, 1024);
    fprintf(stderr, "Server running, accepting connections:\n");
    fprintf(stderr, "IP address is: %s\n", name);
    fprintf(stderr, "port is: %d\n", (int) ntohs(address.sin_port));
    */

   s.connection_listener(address, addrlen);

    // Closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
