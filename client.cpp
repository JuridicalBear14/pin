#include "local.h"

Client::Client(std::string name, int fd) {
    interface = new MessageWindow();
    interface->set_parent(this);
    client_fd = fd;
    this->name = name;
}

void Client::send_message(std::string buf) {
    int sent = send(client_fd, buf.c_str(), buf.size(), 0);
}

void Client::start_interface() {
    interface->start_interface();
}

void Client::set_client_fd(int fd) {
    client_fd = fd;
}

/* Recieve message and write to interface */
void Client::recieve() {
    char buf[MAXMSG];   // Has to be C style to be read into
    memset(buf, 0, sizeof(buf));
    int ret;
    
    while ((ret = read(client_fd, buf, MAXMSG - 1)) > 0) {    // -1 to leave room for null
        pthread_mutex_lock(&mutex);

        interface->update_data(std::string(buf));
        interface->write_to_screen();

        pthread_mutex_unlock(&mutex);
        memset(buf, 0, sizeof(buf));
    }
}