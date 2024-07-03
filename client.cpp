#include "local.h"

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
    read_message(header, buf);

    // Loop and collect messages
    int message_count = header.size;
    for (int i = 0; i < message_count; i++) {
        int ret = read_message(header, buf);
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

    // Send header
    int ret = send(client_fd, &header, sizeof(header), 0);

    // Send message
    int sent = send(client_fd, buf.c_str(), header.size, 0);
}

/* Read a message into str and header */
int Client::read_message(p_header& header, std::string& str) {
    // First read header
    int ret = read(client_fd, &header, sizeof(p_header));

    if (ret <= 0) {
        return 0;
    }

    int size = header.size % MAXMSG;    // % MAXMSG in case somehow bigger than max

    // If something else to read, read it
    if (size > 0 && header.status != STATUS_CONNECT) {
        char buf[size + 1];   // +1 to allow space for null byte
        memset(buf, 0, sizeof(buf));

        int r = read(client_fd, buf, header.size);

        str.assign(buf);
    }

    return size;
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
    
    while (read_message(header, str) > 0) {    // -1 to leave room for null
        interface->update_data(str);
        interface->write_to_screen();
    }
}