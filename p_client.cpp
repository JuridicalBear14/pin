#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "defn.h"

int client_fd;
char* name;
int namelen;

void send_message(char* buf, int size) {
    //char msg[size + namelen + 3];

    //snprintf(msg, size + namelen + 3, "<%s> %s", name, buf);
    send(client_fd, buf, size, 0);
}

void* recieve(void* argv) {
    char buf[MAXMSG];

    while (read(client_fd, buf, MAXMSG) != 0) {
        pthread_mutex_lock(&mutex);

        add_remote(buf, strlen(buf));
        write_messages();

        pthread_mutex_unlock(&mutex);
        memset(buf, 0, sizeof(buf));
    }

    return NULL;
}

// Init function run on connection
void init() {
    // Send name over
    send(client_fd, name, NAMELEN, 0);
}

int main(int argc, char** argv) {
    char* ip = "127.0.0.1";
    name = "NULL";

    // Name given
    if (argc > 1) {
        name = argv[1];
        if (strlen(name) > NAMELEN) {  // Clip off name
            name[NAMELEN] = 0;
        }
    }

    namelen = strlen(name);

    // ip given
    if (argc > 2) {
        ip = argv[2];
    }

    int status, valread;
    struct sockaddr_in serv_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    init();

    pthread_t listener;
    pthread_t interface;

    pthread_create(&listener, NULL, recieve, NULL);
    pthread_create(&interface, NULL, start_interface, NULL);

    // Cleanup threads
    pthread_detach(listener);    // Detach this guy, since we only want to wait on local
    pthread_join(interface, NULL);

    // closing the connected socket
    close(client_fd);
    return 0;
}
