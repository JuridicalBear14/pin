//#define _GNU_SOURCE   // Not sure why this was here

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include "defn.h"

// Array of fds to monitor
struct pollfd pollfds[MAXUSR];
char* names[MAXUSR];

// Find next open port index
int nextindex() {
    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd == -1) {
            return i;
        }
    }

    // No open slots
    return -1;
}

// Send message to all used ports other than given
void sendall(int ix, char* buf) {
    // Construct message
    char* name = names[ix];
    int msg_size = strlen(buf) + 4 + strlen(name);
    char* msg = (char*) malloc(msg_size);
    snprintf(msg, msg_size, "<%s> %s", name, buf);

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && i != ix) {
            // Send to socket
            write(pollfds[i].fd, msg, msg_size);
        }
    }
}

// Initialization function called on every new connection
void init(int ix) {
    // Read name
    names[ix] = (char*) malloc(sizeof(char) * NAMELEN);   // name char limit
    read(pollfds[ix].fd, names[ix], NAMELEN);
}

// Monitor and relay socket messages
void* relay(void* argv) {
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

    return NULL;
}

// Main startup and port accept function
int main(void) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    fprintf(stderr, "Starting server...\n");

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

    // Set up pollfds
    for (int i = 0; i < MAXUSR; i++) {
        pollfds[i].events = POLLIN;
        pollfds[i].fd = -1;
    }

    // Set up relay thread
    pthread_t r_thread;
    pthread_create(&r_thread, NULL, relay, NULL);

    // Playing with printing IP
    /*
    fprintf(stderr, "Server running, accepting connections:\n");
    fprintf(stderr, "IP address is: %s\n", inet_ntoa(address.sin_addr));
    fprintf(stderr, "port is: %d\n", (int) ntohs(address.sin_port));
    */

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

        init(index);
        fprintf(stderr, "Connection accepted in slot %d, fd: %d\n", index, pollfds[index].fd);
    }

    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
