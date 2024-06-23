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
    if (size > 0) {
        char buf[size + 1];   // +1 to allow space for null byte
        memset(buf, 0, sizeof(buf));

        read(client_fd, buf, sizeof(buf));
        str.assign(buf);
    }

    return size;
}

void Client::start_interface() {
    interface->start_interface();
}

void Client::set_client_fd(int fd) {
    client_fd = fd;
}

/* Recieve message and write to interface */
void Client::recieve() {
    std::string str;
    p_header header;
    
    while (read_message(header, str) > 0) {    // -1 to leave room for null
        pthread_mutex_lock(&mutex);
        
        interface->update_data(str);
        interface->write_to_screen();

        pthread_mutex_unlock(&mutex);
    }
}