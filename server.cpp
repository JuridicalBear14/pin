#include "server.h"

Server::Server(int fd) {
    server_fd = fd;

    // Set up pollfds
    for (int i = 0; i < MAXUSR; i++) {
        pollfds[i].events = POLLIN;
        pollfds[i].fd = -1;
    }
}

/* Start up the server (for now just calls relay, but will be expanded upon later) */
void Server::start_server() {
    msg_relay();
}

/* Listen and accept incoming connections */
void Server::connection_listener(struct sockaddr_in address, int addrlen) {
    // Wait and accept incoming connections
    int index;
    while (1) {
        if ((index = nextindex()) == -1) {
            // No open slots
            continue;
        }

        printf("Slot %d available\n", index);

        // Accept connection
        if ((pollfds[index].fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        init_connection(index);
        printf("Connection accepted in slot %d, fd: %d\n", index, pollfds[index].fd);
    }
}

int Server::nextindex() {
    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd == -1) {
            return i;
        }
    }

    // No open slots
    return -1;
}

void Server::sendall(int ix, std::string buf) {
    // Construct message
    std::string msg = "<" + names[ix] + "> " + buf;

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && i != ix) {
            // Send to socket
            int r = write(pollfds[i].fd, msg.c_str(), msg.length());
        }
    }
}

void Server::init_connection(int ix) {
    // Read name
    char buf[NAMELEN];
    memset(buf, 0, sizeof(buf));

    read(pollfds[ix].fd, buf, sizeof(buf));
    names.push_back(std::string(buf));

    std::cout << "Name recieved: " + names[names.size() - 1] + "\n";
}

void Server::msg_relay() {
    int n;
    char buf[MAXMSG];
    memset(buf, 0, sizeof(buf));

    // Wait for event
    while ((n = poll(pollfds, MAXUSR, 1000)) != -1) {

        // Figure out which fd(s) updated
        for (int i = 0; i < MAXUSR; i++) {
            if (pollfds[i].revents & POLLIN) {

                // Ready to read
                if (!read(pollfds[i].fd, buf, sizeof(buf))) {
                    // Closed
                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;

                    printf("Closed slot: %d\n", i);
                    continue;
                }

                std::cout << "Message recieved from user: " + names[i] + "\n";

                // Send message to all others
                sendall(i, std::string(buf));
                memset(buf, 0, sizeof(buf));

                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}