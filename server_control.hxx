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

        // Managers
        void list_manager(std::vector<std::string> tokens);
        int list_users();

        void edit_manager(std::vector<std::string> tokens);


        void create_manager(std::vector<std::string> tokens);
        int create_user(std::string name);

        void delete_manager(std::vector<std::string> tokens);
        int delete_user(std::string name);
        int delete_user(int uid);

        void shutdown();

        Server* server;
};