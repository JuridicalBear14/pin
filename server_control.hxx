#pragma once

#include "defn.hxx"
#include "database.hxx"
#include "net.hxx"
#include "util.hxx"
#include "server.hxx"

class Server;

class Server_control {
    public:
        Server_control(Server* s);
        void start_scontrol();

    private:
        void user_loop();

        void shutdown();
        int list_users();
        int delete_user(std::string name);
        int delete_user(int uid);

        Server* server;
};