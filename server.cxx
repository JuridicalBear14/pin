#include "server.hxx"

Server::Server(int fd) {
    server_fd = fd;

    // Set up pollfds
    for (int i = 0; i < MAXUSR; i++) {
        pollfds[i].events = POLLIN;
        pollfds[i].fd = -1;
    }
}

/* Connect to database */
void Server::connect_db(Database* db) {
    mut.lock();
    database = db;
    mut.unlock();
}

/* Start up the server (for now just calls relay, but will be expanded upon later) */
void Server::start_server() {
    msg_relay();
}

/* Listen and accept incoming connections */
void Server::connection_listener(struct sockaddr_in address, int addrlen) {
    // Wait and accept incoming connections
    int index;
    int fd;
    while (1) {
        if ((index = nextindex()) == -1) {
            // No open slots
            continue;
        }

        printf("Slot %d available\n", index);

        // Accept connection
        if ((fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Set fd safely
        mut.lock();
        pollfds[index].fd = fd;
        mut.unlock();

        init_connection(index);
        printf("Connection accepted in slot %d, fd: %d\n", index, pollfds[index].fd);
    }
}


/* Initial connection to client, read their name and then transfer convo history */
void Server::init_connection(int ix) {
    // Read name
    std::string str;
    p_header header;

    net::read_msg(pollfds[ix].fd, header, str);

    mut.lock();
    names.push_back(str);
    mut.unlock();

    std::cout << "Name recieved: " + names[names.size() - 1] + "\n";

    // Dispatch thread to catch client up so we can get back to listening
    auto handle = std::async(std::launch::async, sync_client_db, database, pollfds[ix].fd);
}

/* Catch the client up on the contents of the db */
void Server::sync_client_db(Database* database, int fd) {
    // Get items
    std::string items;
    database->get_convo_index(items);

    items = "<Convo Options> " + items;

    // Send to client as list
    p_header header;
    header.uid = -1;
    header.cid = -1;
    header.status = STATUS_CONNECT;
    header.size = items.size();

    net::send_msg(fd, header, items);
}

/* Sync client with contents of convo */
void Server::sync_client_convo(Database* database, int fd, int cid) {
    // Get message list
    std::vector<std::string> messages;
    int ret = database->get_all_messages(cid, messages);

    // Send intitial header to tell client how many messages to expect
    p_header header;
    header.uid = -1;
    header.cid = cid;
    header.status = STATUS_ITEM_COUNT;
    header.data = messages.size();

    net::send_header(fd, header);

    // Now build generic header to pack and send each one
    header.status = STATUS_MSG_OLD;
    int count = 0;
    // Loop and send messages
    for (std::string& s : messages) {
        header.size = s.size();
        net::send_msg(fd, header, s);
    }
}

int Server::nextindex() {
    mut.lock();

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd == -1) {
            mut.unlock();
            return i;
        }
    }

    // No open slots
    mut.unlock();
    return -1;
}

/* Send message to all other users */
void Server::sendall(int ix, std::string buf) {
    // Construct header
    p_header header;
    header.uid = -1;   // NOT IMPLEMENTED
    header.cid = -1;   // NOT IMPLEMENTED
    header.status = STATUS_MSG;

    mut.lock();

    // Construct message
    std::string msg = "<" + names[ix] + "> " + buf;
    header.size = msg.length();

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && i != ix) {
            // Send to socket
            net::send_msg(pollfds[i].fd, header, msg);
        }
    }

    mut.unlock();
}

/* Listen for incoming messages and relay them across the network */
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
                if (!net::read_header(pollfds[i].fd, header)) {
                    mut.lock();

                    // Closed
                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;

                    mut.unlock();

                    printf("Closed slot: %d\n", i);
                    continue;
                }

                std::cout << "Message recieved from user: " + names[i] + "\n";

                // Check header
                switch (header.status) {
                    case STATUS_DB_FETCH:
                        // Dispatch thread to catch client up so we can get back to listening
                        sync_client_convo(database, pollfds[i].fd, header.cid);
                        break;

                    case STATUS_MSG: 
                        // Read rest of message
                        net::read_data(pollfds[i].fd, header.size, str);

                        // Write to db
                        std::string msg = "<" + names[i] + "> " + str;
                        database->write_msg(1, header, msg);

                        // Send message to all others
                        sendall(i, str);
                        break;
                }


                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}