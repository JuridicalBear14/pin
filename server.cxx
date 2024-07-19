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

/* User a user's name as an std::string */
std::string Server::get_username(int ix) {
    return std::string(users[ix].name);
}

/* Initial connection to client, read their name and then transfer convo history */
void Server::init_connection(int ix) {
    // Read name
    std::string str;
    p_header header;

    net::read_msg(pollfds[ix].fd, header, str);

    // Lookup user in db (and create if not found)
    int uid = database->get_user_id(str, true);

    if (uid == -1) {
        // I have no idea what to do here
    }

    mut.lock();
    // Set user data
    strncpy(users[ix].name, str.c_str(), NAMELEN + 1);   // +1 for null
    users[ix].uid = uid;
    users[ix].cid = DB_NONE;
    mut.unlock();

    std::cout << "Name recieved: " << str << ", uid: " << users[ix].uid << "\n";

    // Now send back header with uid
    header.uid = uid;
    net::send_header(pollfds[ix].fd, header);

    // Dispatch thread to catch client up so we can get back to listening
    //auto handle = std::async(std::launch::async, sync_client_db, database, pollfds[ix].fd, uid);
}

/* Catch the client up on the contents of the db */
void Server::sync_client_db(Database* database, int fd, int uid) {
    std::cout << "Sync user " << uid << " with db\n";

    // Get items
    std::vector<Convo> items;
    database->get_convo_index(items);

    p_header header;
    header.uid = uid;
    header.status = STATUS_DB_SYNC;
    header.data = items.size();
    header.size = sizeof(Convo);

    // Send all convos
    for (Convo c : items) {
        net::send_msg(fd, header, &c);
    }

    std::cout << "Sync complete\n";
}

/* Sync client with contents of convo */
void Server::sync_client_convo(Database* database, int fd, int cid, int uid) {
    // Get message list
    std::vector<std::string> messages;
    int ret = database->get_all_messages(cid, messages);

    // Send intitial header to tell client how many messages to expect
    p_header header;
    header.uid = uid;
    header.cid = cid;
    header.status = STATUS_DB_FETCH;
    header.data = messages.size();

    //net::send_header(fd, header);

    // Now build generic header to pack and send each one
    header.status = STATUS_MSG_OLD;
    int count = 0;
    // Loop and send messages
    for (std::string& s : messages) {
        header.size = s.size();
        net::send_msg(fd, header, s);
    }

    std::cout << "Sync complete\n";
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

/* Send message to all other users in the same convo */
void Server::sendall(int ix, std::string buf) {
    // Construct header
    p_header header;
    header.uid = users[ix].uid;
    header.cid = users[ix].cid;
    header.status = STATUS_MSG;

    mut.lock();

    // Construct message
    //std::string msg = "<" + get_username(ix) + "> " + buf;
    header.size = buf.length();

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && users[i].cid == header.cid && i != ix) {
            // Send to socket
            net::send_msg(pollfds[i].fd, header, buf);
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

                    // Wipe user entry
                    users[i].uid = -1;
                    users[i].cid = -1;
                    users[i].name[0] = 0;

                    mut.unlock();

                    printf("Closed slot: %d\n", i);
                    continue;
                }

                std::cout << "Message recieved from user: " << get_username(i) <<  ", uid: " << users[i].uid << ", Type: " << header.status << "\n";

                // Check header
                switch (header.status) {
                    case STATUS_DB_FETCH:
                        // Dispatch thread to catch client up so we can get back to listening
                        std::cout << "Sync client " << get_username(i) << " with convo " << header.cid << "\n";
                        users[i].cid = header.cid;
                        sync_client_convo(database, pollfds[i].fd, header.cid, users[i].cid);
                        break;

                    case STATUS_MSG: 
                        // Read rest of message
                        net::read_data(pollfds[i].fd, header.size, str);

                        // Write to db
                        str = "<" + get_username(i) + "> " + str;
                        database->write_msg(header.cid, header, str);

                        // Send message to all others
                        sendall(i, str);
                        break;
                    
                    case STATUS_DB_SYNC:
                        sync_client_db(database, pollfds[i].fd, header.uid);
                        break;
                }


                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}