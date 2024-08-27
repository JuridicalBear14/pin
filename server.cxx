#include "server.hxx"

// MARK: BASE
// ****************************** <Basic setup and utility> ****************************** //

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

/* User a user's name as an std::string */
std::string Server::get_username(int ix) {
    return std::string(users[ix].name);
}

/* Return the next open index for user slots */
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
    return E_NO_SPACE;
}

// ****************************** </Basic setup and utility> ****************************** //




// MARK: New connect
// ****************************** <Incoming connection handler> ************************* //

/* Listen and accept incoming connections */
void Server::connection_listener(struct sockaddr_in address, int addrlen) {
    // Wait and accept incoming connections
    int index;
    int fd;
    int ret;
    while (1) {
        // Accept connection
        if ((fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // If no slots or bad init, send denial header
        if (((index = nextindex()) == E_NO_SPACE) || ((ret = init_connection(fd, index)) != E_NONE)) {
            p_header h;
            h.size = 0;
            h.status = STATUS_CONNECT_DENIED;
            h.data = ret;   // Send error message to client
            
            net::send_header(fd, h);

            printf("Connection denied\n");
            close(fd);
            continue;
        }

        // Set fd safely
        mut.lock();
        pollfds[index].fd = fd;
        mut.unlock();

        printf("Connection accepted in slot %d, fd: %d\n", index, pollfds[index].fd);
    }
}

/* Initial connection to client, read their name and then transfer convo history */
int Server::init_connection(int fd, int ix) {
    // Read name
    std::string str;
    p_header header;
    int ret;

    if ((ret = net::read_header(fd, header)) != E_NONE) {
        return ret;
    }

    str = std::string(header.user.name);

    // Lookup user in db (and create if not found)
    ret = database->get_user_id(header.user, true);

    if (ret != E_NONE) {   // Validation denied, so deny client
        return ret;
    }

    mut.lock();

    users[ix] = header.user;
    
    mut.unlock();

    // Now send back header with user info
    if ((ret = net::send_header(fd, header)) != E_NONE) {
        return ret;
    }

    mut.lock();
    // Remove master key field if used and encrypt dynamic key
    memset(users[ix].master_key, 0, KEYLEN + 1);
    secure::encrypt(users[ix].dynamic_key, KEYLEN);
    mut.unlock();

    std::cout << "Name recieved: " << str << ", uid: " << users[ix].uid << "\n";
    return E_NONE;
}

// ****************************** </Incoming connection handler> ************************* //




// MARK: User service
// ****************************** <User service functions> ************************* //

/* Send message to all other users in the same convo */
void Server::sendall(int ix, std::string buf) {
    // Construct header
    p_header header;
    header.user = users[ix];
    header.status = STATUS_MSG;

    mut.lock();

    // Construct message
    header.size = buf.length();

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && users[i].cid == header.user.cid && i != ix) {
            // Send to socket
            if (net::send_msg(pollfds[i].fd, header, buf) != E_NONE) {
                std::cout << "Failed to send on slot: " << ix << "\n";
            }
        }
    }

    mut.unlock();
}

// ****************************** </User service functions> ************************* //





// MARK: Database
// ****************************** <Database connection functions> ************************* //

/* Connect to database */
void Server::connect_db(Database* db) {
    mut.lock();
    database = db;
    mut.unlock();
}

/* Catch the client up on the contents of the db */
void Server::sync_client_db(Database* database, int fd, User user) {
    std::cout << "Sync user " << user.uid << " with db\n";

    // Get items
    std::vector<Convo> items;
    if (database->get_convo_index(items, user) != E_NONE) {
        // Error reading, so log and continue with empty list
        std::cout << "Could not read convo index\n";
    }

    p_header header;
    header.user = user;
    header.status = STATUS_DB_SYNC;
    header.data = items.size();
    header.size = sizeof(Convo);

    // If no convos, just send empty header
    if (header.data == 0) {
        header.size = 0;

        if (net::send_header(fd, header) != E_NONE) {
            std::cout << "Sync failed\n";
            return;
        }
    }

    // Send all convos
    for (Convo c : items) {
        if (net::send_msg(fd, header, &c) != E_NONE) {
            std::cout << "Sync failed\n";
            return;
        }
    }

    std::cout << "Sync complete\n";
}

/* Sync client with contents of convo */
void Server::sync_client_convo(Database* database, int fd, User user) {
    // Get message list
    std::vector<std::string> messages;
    int ret = database->get_all_messages(user.cid, messages);

    // Send intitial header to tell client how many messages to expect
    p_header header;
    header.user = user;
    header.status = STATUS_DB_FETCH;
    header.data = messages.size();

    // Now build generic header to pack and send each one
    header.status = STATUS_MSG_OLD;
    int count = 0;

    // Loop and send messages
    for (std::string& s : messages) {
        header.size = s.size();

        if (net::send_msg(fd, header, s) != E_NONE) {
            std::cout << "Sync failed\n";
            return;
        }
    }

    std::cout << "Sync complete\n";
}

// ****************************** </Database connection functions> ************************* //



// MARK: Reciever loop
/* Listen for incoming messages and relay them across the network */
void Server::msg_relay() {
    int n;
    int ret;
    std::string str;
    p_header header;
    Convo c;

    // Wait for event
    while ((n = poll(pollfds, MAXUSR, 1000)) != -1) {

        // Figure out which fd(s) updated
        for (int i = 0; i < MAXUSR; i++) {
            if (pollfds[i].revents & POLLIN) {

                // Ready to read
                if ((ret = net::read_header(pollfds[i].fd, header)) == E_CONNECTION_CLOSED) {
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
                } else if (ret != E_NONE) {
                    // Reset revents
                    pollfds[i].revents = 0;
                    continue;
                }

                // Validate the message sender
                if (!secure::validate_user(header.user, users[i])) {
                    // Bad credentials, so disconnect
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
                        std::cout << "Sync client " << get_username(i) << " with convo " << header.user.cid << "\n";
                        users[i].cid = header.user.cid;
                        sync_client_convo(database, pollfds[i].fd, header.user);
                        break;

                    case STATUS_MSG: 
                        // Read rest of message
                        if (net::read_data(pollfds[i].fd, header.size, str) != E_NONE) {
                            // Read failed
                            break;
                        }

                        // Write to db
                        str = "<" + get_username(i) + "> " + str;
                        //std::cout << str << "\n";
                        ret = database->write_msg(header.user.cid, header, str);
                        if (ret != E_NONE && ret != DB_NONE) {
                            std::cout << "Could not write message\n";
                        }

                        // Send message to all others
                        sendall(i, str);
                        break;
                    
                    case STATUS_DB_SYNC:
                        std::cout << "sync\n";
                        sync_client_db(database, pollfds[i].fd, header.user);
                        break;

                    case STATUS_CONVO_CREATE:
                        // Read rest of message
                        if (net::read_data(pollfds[i].fd, header.size, &c) != E_NONE) {
                            // Failed read
                            net::send_header(pollfds[i].fd, {users[i].uid, -1, STATUS_ERROR, 0, 0});
                            break;
                        }

                        // Create the convo
                        ret = database->create_convo(c);

                        // If couldn't write, send back error
                        if (ret < 1) {
                            header.status = STATUS_ERROR;
                            header.size = 0;

                            net::send_header(pollfds[i].fd, header);
                        } else {
                            // Now send back the convo with new cid
                            net::send_msg(pollfds[i].fd, header, &c);
                        }
                        break;
                }


                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}