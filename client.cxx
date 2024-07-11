#include "local.hxx"

Client::Client(std::string name, int fd) {
    interface = new MessageWindow();
    interface->set_parent(this);
    client_fd = fd;
    this->name = name;

    uid = -1;   // NOT IMPLEMENTED
}

/* Create and manage all interfaces */
void Client::interface_handler() {
    // Run until exit code break
    while (true) {
        std::cout << tempbuf << "\n";

        std::string buf;
        std::cin >> buf;

        int choice = std::atoi(buf.c_str());
        cid = choice;

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
    send_message(STATUS_CONNECT, name.c_str());

    // Now recieve message detailing convos
    p_header header;
    std::string buf;

    int s = net::read_msg(client_fd, header, buf);
    tempbuf = buf;
}

/* Request a convo's data from server */
void Client::fetch_convo(std::vector<std::string>& str) {
    // Send request to server
    p_header req;
    req.cid = cid;
    req.size = 0;
    req.status = STATUS_DB_FETCH;
    req.uid = -1;

    net::send_header(client_fd, req);
}

void Client::send_message(int status, std::string buf) {
    // Construct header
    p_header header;
    header.uid = uid;
    header.cid = cid;   // NOT IMPLEMENTED
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

    // Read headers until socket close
    while (net::read_header(client_fd, header) > 0) {
        // Figure out msg type
        switch (header.status) {
            case STATUS_MSG:
            case STATUS_MSG_OLD:
                if (net::read_data(client_fd, header.size, str) > 0) {
                    //std::cout << str.length() << "\n";
                    interface->update_data(str, header.status);
                    interface->write_to_screen();
                }
                break;
        }
    }
}