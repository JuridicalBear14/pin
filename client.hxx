#include "defn.hxx"
#include "net.hxx"

class Interface;

class Client {
    public:
        Client(std::string name, int fd);
        ~Client();
        void send_message(int status, std::string buf);
        void recieve();
        void start_interface();
        void set_client_fd(int fd);
        void init();

        std::string name;

    private:
        Interface* interface;
        int client_fd;
        std::mutex mut;
        int uid;
};