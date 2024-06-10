#include "defn.h"

class Server {
    public:
        Server(int fd);
        void start_server();
        void connection_listener(struct sockaddr_in address, int addrlen);

    private:
        int nextindex();
        void sendall(int ix, char* buf);
        void init_connection(int ix);
        void msg_relay();

        // Array of fds to monitor and names
        struct pollfd pollfds[MAXUSR];
        char* names[MAXUSR];

        int server_fd;
};