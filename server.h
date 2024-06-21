#include "defn.h"

class Server {
    public:
        Server(int fd);
        void start_server();
        void connection_listener(struct sockaddr_in address, int addrlen);

    private:
        int nextindex();
        void sendall(int ix, std::string);
        void init_connection(int ix);
        void msg_relay();
        int readmsg(int fd, p_header& header, std::string& str);
        void send_msg(int fd, p_header header, std::string buf);

        // Array of fds to monitor and names
        struct pollfd pollfds[MAXUSR];
        std::vector<std::string> names;

        int server_fd;
};