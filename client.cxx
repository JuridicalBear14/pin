#include "local.hxx"

Client::Client(std::string name, int fd) {
    interface = new MessageWindow();
    interface->set_parent(this);
    client_fd = fd;
    this->name = name;

    uid = -1;   // NOT IMPLEMENTED
}

Client::~Client() {
    //delete interface;   // Make sure to clean up allocated interface when we go out
}

/* Initialize connection to server */
void Client::init() {
    // Send name over
    send_message(STATUS_CONNECT, name.c_str());

    // Now recieve header detailing how many messages
    std::string buf;
    p_header header;
    net::read_header(client_fd, header);

    // Loop and collect messages
    int message_count = header.size;
    for (int i = 0; i < message_count; i++) {
        int ret = net::read_msg(client_fd, header, buf);
        interface->update_data(buf);
    }
}

void Client::send_message(int status, std::string buf) {
    // Construct header
    p_header header;
    header.uid = uid;
    header.cid = -1;   // NOT IMPLEMENTED
    header.status = status;
    header.size = buf.length();

    // Now call net
    int ret = net::send_msg(client_fd, header, buf);
}

void Client::start_interface() {
    interface->start_interface();
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