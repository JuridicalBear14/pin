#include "defn.h"
#include "database.h"

// Constants
#define SERVER_SETTINGS_FILE "settings.conf"

// Server settings struct
struct server_settings {
    int db;    // ID of which db to use
    int port;
};

class Server {
    public:
        Server(int fd);
        void start_server();
        void connection_listener(struct sockaddr_in address, int addrlen);
        void connect_db(Database* database);

    private:
        int nextindex();
        void sendall(int ix, std::string);
        void init_connection(int ix);
        void msg_relay();
        static int readmsg(int fd, p_header& header, std::string& str);
        static void send_msg(int fd, p_header header, std::string buf);
        static void send_header(int fd, p_header header);
        static void sync_client_db(Database* database, int fd);

        std::mutex mut;

        // Array of fds to monitor and names
        struct pollfd pollfds[MAXUSR];
        std::vector<std::string> names;

        // Connected database
        Database* database;

        int server_fd;
};