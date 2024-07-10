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

        std::vector<std::string> s;
        read_convo(choice, s);

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

    net::read_msg(client_fd, header, buf);
    tempbuf = buf;
}

/* Read a convo's data from server */
void Client::read_convo(int cid, std::vector<std::string>& str) {
    // Send request to server
    p_header req;
    req.cid = cid;
    req.size = 0;
    req.status = STATUS_DB_FETCH;
    req.uid = -1;

    net::send_header(client_fd, req);

    // Read how many messages to expect
    std::string buf;
    p_header header;
    net::read_header(client_fd, header);

    // Loop and collect messages
    int message_count = header.data;
    for (int i = 0; i < message_count; i++) {
        int ret = net::read_msg(client_fd, header, buf);
        std::cout << "read\n";
        interface->update_data(buf);
        std::cout << "update\n";
    }
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
    
    while (net::read_msg(client_fd, header, str) > 0) {
        interface->update_data(str);
        interface->write_to_screen();
    }
}