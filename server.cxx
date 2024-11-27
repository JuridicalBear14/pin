#include "server.hxx"

// MARK: BASE
// ****************************** <Basic setup and utility> ****************************** //

Server::Server(int fd) {
    server_fd = fd;

    // Set up pollfds
    for (int i = 0; i < MAXUSR; i++) {
        pollfds[i].events = POLLIN;
        pollfds[i].fd = SERVER_SLOT_EMPTY;
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
        if (pollfds[i].fd == SERVER_SLOT_EMPTY) {
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
            //perror("accept");
            exit(EXIT_FAILURE);
        }

        // Now that we accepted, set timeout for sending and recieving
        struct timeval tv;
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof(tv)) || setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*) &tv, sizeof(tv))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // If no slots or bad init, send denial header
        if (((index = nextindex()) == E_NO_SPACE) || ((ret = init_connection(fd, index)) != E_NONE)) {
            p_header h;
            h.size = 0;
            h.status = STATUS_CONNECT_DENIED;
            h.data = ret;   // Send error message to client
            
            net::send_header(fd, h);

            util::log("Connection denied: ", util::error2str(ret));
            close(fd);
            continue;
        }

        // Set fd safely
        mut.lock();
        pollfds[index].fd = fd;
        mut.unlock();

        util::log(index, pollfds[index].fd);
    }
}

/* Initial connection to client, read their name and then transfer convo history */
int Server::init_connection(int fd, int ix) {
    // Read name
    p_header header;
    int ret;

    if ((ret = net::read_header(fd, header)) != E_NONE) {
        return ret;
    }

    // If it was just a ping, then return
    if (header.status == STATUS_PING) {
        return E_CONNECTION_CLOSED;
    }

    // If DB_NONE, just accept connection
    if (database->db_none()) {
        header.user.uid = DB_NONE;

        mut.lock();

        users[ix] = header.user;
    
        mut.unlock();

        // Now send back header with user info
        if ((ret = net::send_header(fd, header)) != E_NONE) {
            return ret;
        }

        util::log(users[ix].uid, get_username(ix), "User connected");
        return E_NONE;
    }

    bool newuser = false;  // Bool for whether or not our user is new

    if (header.user.dynamic_key[0] == 0) {   // If key is null, then new user
        newuser = true;
    }

    // Lookup user in db (and create if not found)
    ret = database->get_user_id(header.user, newuser);

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

    util::log(users[ix].uid, get_username(ix), "User connected");
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
    int err;

    mut.lock();

    // Construct message
    header.size = buf.length();

    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1 && users[i].cid == header.user.cid && i != ix) {
            // Send to socket
            if ((err = net::send_msg(pollfds[i].fd, header, buf)) != E_NONE) {
                util::error(err, "Failed to send on slot " + i);
            }
        }
    }

    mut.unlock();
}

/* Disconnect all current users, can close slot permanently or just disconnect */
void Server::disconnect_all(bool toclose) {
    // Construct header
    p_header header;
    header.status = STATUS_DISCONNECT;
    
    int err;
    for (int i = 0; i < MAXUSR; i++) {
        if (pollfds[i].fd != -1) {   // Valid user
            header.user = users[i];

            // Send to socket
            if ((err = net::send_header(pollfds[i].fd, header)) != E_NONE) {
                util::error(err, "Failed to send on slot " + i);
            }

            mut.lock();

            // Closed
            close(pollfds[i].fd);

            if (toclose) {
                pollfds[i].fd = SERVER_SLOT_CLOSED;
            } else {
                pollfds[i].fd = SERVER_SLOT_EMPTY;
            }

            // Wipe user entry
            users[i].uid = -1;
            users[i].cid = -1;
            users[i].name[0] = 0;

            mut.unlock();

            util::log("Closed slot: ", i);
        }
    }
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
    util::log("DB sync with user: ", user.uid);
    int err;

    // Get items
    std::vector<Convo> items;
    if ((err = database->get_convo_index(items, user, false)) != E_NONE) {
        // Error reading, so log and continue with empty list
        util::error(err, "Could not read convo index");
    }

    p_header header;
    header.user = user;
    header.status = STATUS_DB_SYNC;
    header.data = items.size();
    header.size = util::ssize(items[0]);   // Size should be for serialized objects

    // If no convos, just send empty header
    if (header.data == 0) {
        header.size = 0;

        if ((err = net::send_header(fd, header)) != E_NONE) {
            util::error(err, "Sync failed");
            return;
        }
    }

    // Send all convos, each convo needs to be serialized
    char buf[util::ssize(items[0])];   // Safe to assume at least one item in vector
    for (Convo c : items) {
        if (util::serialize(buf, util::ssize(c), c) != E_NONE) {
            util::error(E_FAILED_WRITE, "Sync failed");
            return;
        }

        if ((err = net::send_msg(fd, header, buf)) != E_NONE) {
            util::error(err, "Sync failed");
            return;
        }
    }

    util::log("Database sync complete for user: ", user.uid);
}

/* Sync client with contents of convo */
void Server::sync_client_convo(Database* database, int fd, User user) {
    int err;

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

        if ((err = net::send_msg(fd, header, s)) != E_NONE) {
            util::error(err, "Convo sync failed");
            return;
        }
    }
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
    char buf[util::ssize(c)];

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
                    pollfds[i].fd = SERVER_SLOT_EMPTY;

                    // Wipe user entry
                    users[i].uid = -1;
                    users[i].cid = -1;
                    users[i].name[0] = 0;

                    mut.unlock();

                    util::log("Closed slot: ", i);
                    continue;
                } else if (ret != E_NONE) {
                    // Reset revents
                    pollfds[i].revents = 0;
                    continue;
                }

                // Validate the message sender
                if (!database->db_none() && !secure::validate_user(header.user, users[i])) {
                    // Bad credentials, so disconnect
                    mut.lock();

                    // Closed
                    close(pollfds[i].fd);
                    pollfds[i].fd = SERVER_SLOT_EMPTY;

                    // Wipe user entry
                    users[i].uid = -1;
                    users[i].cid = -1;
                    users[i].name[0] = 0;

                    mut.unlock();

                    util::log("Closed slot: ", i);
                    continue;
                }

                //std::cout << "Message recieved from user: " << get_username(i) <<  ", uid: " << users[i].uid << ", Type: " << header.status << "\n";
                util::log(header.status, header.user.uid, get_username(i), header.user.cid);

                // Check header
                switch (header.status) {
                    case STATUS_DB_FETCH:
                        // Dispatch thread to catch client up so we can get back to listening
                        users[i].cid = header.user.cid;
                        sync_client_convo(database, pollfds[i].fd, header.user);
                        break;

                    case STATUS_MSG: 
                        // Read rest of message
                        if ((ret = net::read_data(pollfds[i].fd, header.size, str)) != E_NONE) {
                            // Read failed
                            util::error(ret, "Unable to read message data");
                            break;
                        }

                        // Write to db
                        str = "<" + get_username(i) + "> " + str;
                        //std::cout << str << "\n";
                        ret = database->write_msg(header.user.cid, header, str);
                        if (ret != E_NONE && ret != DB_NONE) {
                            util::error(ret, "Could not write message to DB");
                        }

                        // Send message to all others
                        sendall(i, str);
                        break;
                    
                    case STATUS_DB_SYNC:
                        sync_client_db(database, pollfds[i].fd, header.user);
                        break;

                    case STATUS_CONVO_CREATE:
                        // Read rest of message
                        if (net::read_data(pollfds[i].fd, header.size, buf) != E_NONE) {
                            // Failed read
                            net::send_header(pollfds[i].fd, {users[i].uid, -1, STATUS_ERROR, 0, 0});
                            break;
                        }

                        // Deserialize convo
                        if (util::deserialize(buf, c) != E_NONE) {
                            // Failed read
                            net::send_header(pollfds[i].fd, {users[i].uid, -1, STATUS_ERROR, 0, 0});
                            break;
                        }

                        // Create the convo
                        ret = database->create_convo(c);

                        // If couldn't write, send back error
                        if (ret < 1) {
                            header.data = STATUS_ERROR;
                            header.size = 0;

                            net::send_header(pollfds[i].fd, header);
                        } else {
                            // Now send back the convo with new cid
                            if (util::serialize(buf, sizeof(buf), c) != E_NONE) {
                                break;
                            }

                            net::send_msg(pollfds[i].fd, header, buf);
                        }
                        break;
                }


                // Reset revents
                pollfds[i].revents = 0;
            }
        }
    }
}