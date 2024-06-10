#include "client.h"

/* Init function run on connection */
void init(Client c, int client_fd) {
    // Send name over
    int sent = send(client_fd, c.name, NAMELEN, 0);
}

/* Pass control to interface class */
void* start_interface(void* args) {
    Client c = *((Client*) args);
    c.start_interface();

    return NULL;
}

/* Pass control to client class */
void* start_listener(void* args) {
    Client c = *((Client*) args);
    c.recieve();

    return NULL;
}


int main(int argc, char** argv) {
    int client_fd;
    char* ip = "127.0.0.1";
    char* name = "NULL";

    // Name given
    if (argc > 1) {
        name = argv[1];
        if (strlen(name) > NAMELEN) {  // Clip off name
            name[NAMELEN] = 0;
        }
    }

    int namelen = strlen(name);

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
    
    Client c(name, client_fd);
    init(c, client_fd);

    pthread_t listener_t;
    pthread_t interface_t;

    pthread_create(&interface_t, NULL, start_interface, &c);
    pthread_create(&listener_t, NULL, start_listener, &c);

    // Cleanup threads
    pthread_detach(listener_t);    // Detach this guy, since we only want to wait on local
    pthread_join(interface_t, NULL);

    // closing the connected socket
    close(client_fd);
    return 0;
}
