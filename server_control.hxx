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
        void list_users(bool all);
        void list_convos();

        void edit_manager(std::vector<std::string> tokens);


        void create_manager(std::vector<std::string> tokens);
        int create_user(std::string name);
        int create_convo(std::vector<std::string> tokens);

        void delete_manager(std::vector<std::string> tokens);
        void delete_user(std::string name);
        void delete_convo(int cid);
        void delete_db(int id);

        void shutdown();
        void disconnect();

        Server* server;
};