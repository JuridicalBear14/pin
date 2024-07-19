#include "local.hxx"

Client::Client(std::string name, int fd) {
    interface = new MessageWindow();
    interface->set_parent(this);
    client_fd = fd;
    user.cid = -1;

    // Set name
    strncpy(user.name, name.c_str(), NAMELEN + 1);   // +1 for null
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

        // Make sure it's an int

        int choice = std::atoi(buf.c_str());
        user.cid = choice;

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

/* Initialize connection to server */
void Client::init() {
    // Send name over
    send_message(STATUS_CONNECT, user.name);

    // Recieve user id
    p_header header;
    net::read_header(client_fd, header);
    user.uid = header.uid;
}

/* Fetch all convo options from server and return the count */
int Client::fetch_convo_options(std::vector<Convo>& v) {
    // First we send the request
    p_header header;
    header.uid = user.uid;
    header.status = STATUS_DB_SYNC; 

    std::unique_lock<std::mutex> lock(mut);

    net::send_header(client_fd, header);

    // Then wait for the socket to recieve every packet
    convo_waiter.wait(lock);

    // Now transfer to arg vector and clear convo_transfer
    v = convo_vector;
    convo_vector.clear();

    mut.unlock();
    return v.size();
}

/* Request a convo's data from server */
void Client::request_convo(std::vector<std::string>& str) {
    // Send request to server
    p_header req;
    req.cid = user.cid;
    req.size = 0;
    req.status = STATUS_DB_FETCH;
    req.uid = -1;

    net::send_header(client_fd, req);
}

void Client::send_message(int status, std::string buf) {
    // Construct header
    p_header header;
    header.uid = user.uid;
    header.cid = user.cid;   // NOT IMPLEMENTED
    header.status = status;
    header.size = buf.length();

    // Now call net
    int ret = net::send_msg(client_fd, header, buf);
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
    while (net::read_header(client_fd, header) > 0) {
        // Figure out msg type
        switch (header.status) {
            case STATUS_MSG:
            case STATUS_MSG_OLD:
                if (net::read_data(client_fd, header.size, str) > 0) {
                    interface->update_data(str, header.status);
                    interface->write_to_screen();
                }
                break;
            case STATUS_DB_SYNC:
                
                // Read into vector
                if (net::read_data(client_fd, header.size, &c) > 0) {
                    mut.lock();
                    convo_vector.push_back(c);
                    mut.unlock();

                    // If size equals total size, then we're done
                    if (convo_vector.size() >= header.data) {
                        convo_waiter.notify_all();
                    }
                }
        }
    }
}