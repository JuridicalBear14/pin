#include "local.h"

Client::Client(char* name, int fd) {
    interface = new MessageWindow();
    interface->set_parent(this);
    client_fd = fd;
    this->name = name;
}

void Client::send_message(char* buf, int size) {
    int sent = send(client_fd, buf, size, 0);
}

void Client::start_interface() {
    interface->start_interface();
}

void Client::set_client_fd(int fd) {
    client_fd = fd;
}

/* Recieve message and write to interface */
void Client::recieve() {
    char buf[MAXMSG];
    int ret;
    while ((ret = read(client_fd, buf, MAXMSG)) > 0) {
        pthread_mutex_lock(&mutex);
        fflush(stderr);    // Not sure why I need this, fix later
        interface->update_data(buf, strlen(buf));
        interface->write_to_screen();

        pthread_mutex_unlock(&mutex);
        memset(buf, 0, sizeof(buf));
    }
}