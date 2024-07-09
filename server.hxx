#include "defn.hxx"
#include "database.hxx"
#include "net.hxx"

// Constants
#define SERVER_SETTINGS_FILE "pin.conf"

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
        static void sync_client_db(Database* database, int fd);
        static void sync_client_convo(Database* database, int fd, int cid);

        std::mutex mut;

        // Array of fds to monitor and names
        struct pollfd pollfds[MAXUSR];
        std::vector<std::string> names;

        // Connected database
        Database* database;

        int server_fd;
};