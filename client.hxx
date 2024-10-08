#pragma once

#include "defn.hxx"
#include "net.hxx"
#include "secure.hxx"

class Interface;


/*
 * Core client class, handles all interaction with the server and manages the interfaces.
*/
class Client {
    public:
        Client();
        void interface_handler();
        int send_message(int status, std::string buf);
        void recieve();
        int init(int fd);
        int request_convo(std::vector<std::string>& str);
        int fetch_convo_options(std::vector<Convo>& v);
        void user_login(std::string name, std::string key);
        std::string getname();
        Convo getconvo();
        std::string getkey();

    private:
        int request_new_convo(Convo c);
        int build_new_convo(Convo& c);
        int send_and_wait(p_header header, void* buf, std::condition_variable* waiter);

        Interface* interface;
        int client_fd;
        std::mutex mut;
        User user;
        Convo convo;
        
        // Vars to synchronizing convos
        std::vector<Convo> convo_vector;
        std::condition_variable convo_waiter;
};