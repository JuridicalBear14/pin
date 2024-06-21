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

/* Read a message into str and return the header */
int Server::readmsg(int fd, p_header& header, std::string& str) {
    // First read header
    int ret = read(fd, &header, sizeof(p_header));
    if (ret <= 0) {
        return 0;
    }

    int size = header.size % MAXMSG;    // % MAXMSG in case somehow bigger than max

    // If something else to read, read it
    if (size > 0) {
        char buf[size + 1];   // +1 to allow space for null byte
        memset(buf, 0, sizeof(buf));

        read(fd, buf, sizeof(buf));
        str.assign(buf);
    }

    return size;
}

/* Send a message to a client */
void Server::send_msg(int fd, p_header header, std::string buf) {
    // Send header
    int ret = send(fd, &header, sizeof(header), 0);

    // Send message
    int sent = send(fd, buf.c_str(), header.size, 0);
}

void Server::sendall(int ix, std::string buf) {
    // Construct header
    p_header header;
    header.uid = -1;   // NOT IMPLEMENTED
    header.cid = -1;   // NOT IMPLEMENTED
    header.status = STATUS_MSG;

    // Construct message
    std::string msg = "<" + names[ix] + "> " + buf;
    header.size = msg.length();

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && i != ix) {
            // Send to socket
            send_msg(pollfds[i].fd, header, msg);
        }
    }
}

void Server::init_connection(int ix) {
    // Read name
    std::string str;
    p_header header;

    readmsg(pollfds[ix].fd, header, str);
    names.push_back(str);

    std::cout << "Name recieved: " + names[names.size() - 1] + "\n";
}

void Server::msg_relay() {
    int n;
    std::string str;
    p_header header;

    // Wait for event
    while ((n = poll(pollfds, MAXUSR, 1000)) != -1) {

        // Figure out which fd(s) updated
        for (int i = 0; i < MAXUSR; i++) {
            if (pollfds[i].revents & POLLIN) {

                // Ready to read
                if (!readmsg(pollfds[i].fd, header, str)) {
                    // Closed
                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;

                    printf("Closed slot: %d\n", i);
                    continue;
                }

                std::cout << "Message recieved from user: " + names[i] + "\n";

                // Send message to all others
                sendall(i, str);

                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}