#include "local.hxx"

Client::Client() {
    interface = new MessageWindow();
    interface->set_parent(this);
    user.cid = -1;
    memset(user.key, 0, sizeof(user.key));
}

/* Get user login info from terminal before anything else */
void Client::user_login(std::string name, std::string key) {
    // First get name
    if (name.size() == 0) {   // If already in, skip
        std::string s;

        // Loop until valid selection
        while (s != "n" && s != "r") {
            // Ask if new user
            std::cout << "[N]ew user or [R]eturning user?\n";
            
            std::cin >> s;
            s = std::tolower(s.c_str()[0]);   // Obnoxious process to lowercase a string
        }

        std::cout << "Please input name: ";
        std::cin >> name;

        // If a new user, skip asking for key
        if (s == "n") {
            // Set name
            strncpy(user.name, name.c_str(), NAMELEN + 1);   // +1 for null
            return;
        }
    }

    // Set name
    strncpy(user.name, name.c_str(), NAMELEN + 1);   // +1 for null

    // Now get key (in a loop to make sure they put it in right)
    while (key.size() != KEYLEN) {
        std::cout << "Please input user key:";

        ////////////////////////// TURN OFF TERMINAL ECHO HERE //////////////////////////////////

        std::cin >> key;

        /////////////////////////// TURN ON TERMINAL ECHO HERE /////////////////////////////////

        /////////////////////////// ENCRYPT KEY HERE? //////////////////////////////////////

        // Now set the key
        strncpy(user.key, key.c_str(), NAMELEN + 1);   // +1 for null
    }
}

/* Get username as a std::string */
std::string Client::getname() {
    return std::string(user.name);
}

/* Create and manage all interfaces */
void Client::interface_handler() {
    // Run until exit code break
    while (true) {
        // First we fetch all convo options
        std::vector<Convo> options;
        int count = fetch_convo_options(options);

        std::cout << "Convo option(s): \n";
        // Now list options to user
        for (int i = 1; i <= count; i++) {
            std::cout << "[" << i << "] " << options[i - 1].name << " | ";
        }
        std::cout << "[0] Create New\n";

        std::string buf;
        std::cin >> buf;

        // Check if quit
        if (buf.length() == 1 && buf[0] == 'q') {
            break;
        }

        char* p;
        long choice = std::strtol(buf.c_str(), &p, 10);

        // Make sure it didn't fail
        if (*p || choice < 0 || choice > count) {
            std::cout << "\nPlease input a valid number\n";
            continue;
        } else if (choice == 0) {   // New convo
            Convo c;
            int ret = build_new_convo(c);

            // Check for quit
            if (ret != E_NONE) {
                break;
            }
            
            // Now request a new convo with this struct
            user.cid = request_new_convo(c);
            
            if (user.cid == E_NO_SPACE) {
                std::cout << "Failed to create new convo\n";
                continue;
            }

        } else {   // Normal
            user.cid = options[choice - 1].cid;   // Remember to get cid from array since numbers are just for selection
        }

        int ret = interface->start_interface();

        if (ret == EXIT_NONE) {
            break;
        }

        // Just for testing build new interface each time
        delete interface;
        interface = new MessageWindow();
        interface->set_parent(this);
    }
}

/* Build a convo from user options, return -1 for quit */
int Client::build_new_convo(Convo& c) {
    std::string buf;

    // Get name from user
    std::cout << "Please enter name for new convo (15 character max, no spaces):\n";
    std::cin >> buf;

    // Shorten to 15 chars (if necessary)
    if (buf.size() > 15) {
        buf.substr(0, 14);
    }

    c.cid = -1;
    c.global = true;    // NOT IMPLEMENTED
    strncpy(c.name, buf.c_str(), NAMELEN);
    c.name[NAMELEN] = 0;   // Make sure null at the end

    return E_NONE;
}

/* Initialize connection to server and authenticate user */
int Client::init(int fd) {
    client_fd = fd;

    int ret;

    // Send user struct over
    p_header header;
    header.user = user;
    header.size = 0;
    header.status = STATUS_CONNECT;
    if (ret = net::send_header(client_fd, header) != E_NONE) {
        return ret;
    }

    // Recieve user id
    if (ret = net::read_header(client_fd, header) != E_NONE) {
        return ret;
    }

    // Check for denial
    if (header.status == STATUS_CONNECT_DENIED || header.status == STATUS_ERROR) {
        // Not sure what to do here, for now we exit
        return E_CONNECTION_CLOSED;
    }

    this->user = header.user;

    return E_NONE;
}

/* Fetch all convo options from server and return the count */
int Client::fetch_convo_options(std::vector<Convo>& v) {
    // First we send the request
    p_header header;
    header.user = user;
    header.size = 0;
    header.status = STATUS_DB_SYNC; 

    int ret = send_and_wait(header, NULL, &convo_waiter);

    if (ret != E_NONE) {
        std::cout << "err" << ret << "\n";
        return 0;
    }

    // Now transfer to arg vector and clear convo_transfer
    v = convo_vector;
    convo_vector.clear();

    mut.unlock();
    return v.size();
}

/* Request a convo's data from server */
int Client::request_convo(std::vector<std::string>& str) {
    // Send request to server
    p_header req;
    req.user = user;
    req.size = 0;
    req.status = STATUS_DB_FETCH;

    return net::send_header(client_fd, req);
}

/* Request the server to make a new convo and return its cid */
int Client::request_new_convo(Convo c) {
    // Then we send the request
    p_header header;
    header.user = user;
    header.size = sizeof(c);
    header.status = STATUS_CONVO_CREATE; 

    int ret = send_and_wait(header, &c, &convo_waiter);

    if (ret != E_NONE) {
        return E_NO_SPACE;
    }

    mut.lock();

    // If vector empty then couldn't create
    if (convo_vector.size() == 0) {
        return E_NO_SPACE;
    }

    // Now transfer to arg vector and clear convo_transfer
    c = convo_vector[0];   // Should only be 1 entry
    convo_vector.clear();

    mut.unlock();

    return c.cid;
}

int Client::send_message(int status, std::string buf) {
    // Construct header
    p_header header;
    header.user = user;
    header.status = status;
    header.size = buf.length();

    // Now call net
    return net::send_msg(client_fd, header, buf);
}

/* Send a message and then wait for reply */
int Client::send_and_wait(p_header header, void* buf, std::condition_variable* waiter) {
    std::unique_lock<std::mutex> lock(mut);
    int ret;

    if (buf == NULL) {
        ret = net::send_header(client_fd, header);
    } else {
        ret = net::send_msg(client_fd, header, buf);
    }

    if (ret != E_NONE) {
        mut.unlock();
        return ret;
    }

    // Then wait for the socket to recieve every packet
    convo_waiter.wait(lock);
    mut.unlock();
    return E_NONE;
}

void Client::set_client_fd(int fd) {
    mut.lock();
    client_fd = fd;
    mut.unlock();
}

/* Recieve message and write to interface */
void Client::recieve() {
    std::string str;
    p_header header;
    Convo c;

    // Read headers until socket close
    while (net::read_header(client_fd, header) == E_NONE) {
        // Figure out msg type
        switch (header.status) {
            case STATUS_MSG:
            case STATUS_MSG_OLD:
                if (net::read_data(client_fd, header.size, str) == E_NONE) {
                    interface->update_data(str, header.status);
                    interface->write_to_screen();
                }
                break;
            case STATUS_DB_SYNC:
                // If empty
                if (header.data == 0) {
                    convo_waiter.notify_all();
                    break;
                }

                // Read into vector
                if (net::read_data(client_fd, header.size, &c) == E_NONE) {
                    mut.lock();
                    convo_vector.push_back(c);
                    mut.unlock();

                    // If size equals total size, then we're done
                    if (convo_vector.size() >= header.data) {
                        convo_waiter.notify_all();
                    }
                }
                break;

            case STATUS_CONVO_CREATE:
                // Check for error
                if (header.status == STATUS_ERROR) {
                    convo_waiter.notify_all();
                    break;
                }

                // Read into vector
                if (net::read_data(client_fd, header.size, &c) == E_NONE) {
                    mut.lock();
                    convo_vector.push_back(c);
                    mut.unlock();
                }

                // Notify interface to continue
                convo_waiter.notify_all();
                break;
        }
    }
}