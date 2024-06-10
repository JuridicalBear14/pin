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

        fprintf(stderr, "Slot %d available\n", index);

        // Accept connection
        if ((pollfds[index].fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        init_connection(index);
        fprintf(stderr, "Connection accepted in slot %d, fd: %d\n", index, pollfds[index].fd);
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

void Server::sendall(int ix, char* buf) {
    // Construct message
    char* name = names[ix];
    int msg_size = strlen(buf) + 4 + strlen(name);
    char* msg = (char*) malloc(msg_size);
    snprintf(msg, msg_size, "<%s> %s", name, buf);

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && i != ix) {
            // Send to socket
            int r = write(pollfds[i].fd, msg, msg_size);
        }
    }
}

void Server::init_connection(int ix) {
    // Read name
    names[ix] = (char*) malloc(sizeof(char) * NAMELEN);   // name char limit
    read(pollfds[ix].fd, names[ix], NAMELEN);
    printf("Name recieved: %s\n", names[ix]);
}

void Server::msg_relay() {
    int n;
    char buf[MAXMSG];

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

                    fprintf(stderr, "Closed slot: %d\n", i);
                    continue;
                }

                fprintf(stderr, "Message recieved from user: %s\n", names[i]);

                // Send message to all others
                sendall(i, buf);
                memset(buf, 0, sizeof(buf));

                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}