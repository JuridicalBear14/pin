#include "defn.hxx"
#include "net.hxx"

class Interface;

class Client {
    public:
        Client(std::string name, int fd);
        void interface_handler();
        void send_message(int status, std::string buf);
        void recieve();
        void set_client_fd(int fd);
        void init();
        void fetch_convo(std::vector<std::string>& str);
        int fetch_convo_options(std::vector<Convo>& v);
        std::string getname();

    private:
        Interface* interface;
        int client_fd;
        std::mutex mut;
        User user;

        std::string tempbuf;
};