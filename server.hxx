#pragma once

#include "defn.hxx"
#include "database.hxx"
#include "net.hxx"
#include "util.hxx"
#include "server_control.hxx"

// Constants
#define SERVER_SETTINGS_FILE "pin.conf"

// Server settings struct
struct server_settings {
    int db;    // ID of which db to use
    int port;
};

class Server {
    friend class Server_control;

    public:
        Server(int fd);
        void start_server();
        void connection_listener(struct sockaddr_in address, int addrlen);
        void connect_db(Database* database);

    private:
        int nextindex();
        void sendall(int ix, std::string);
        int init_connection(int fd, int ix);
        void msg_relay();
        static void sync_client_db(Database* database, int fd, User user);
        static void sync_client_convo(Database* database, int fd, User user);
        std::string get_username(int ix);

        std::mutex mut;

        // Array of fds to monitor and names
        struct pollfd pollfds[MAXUSR];
        User users[MAXUSR];

        // Connected database
        Database* database;

        int server_fd;
};