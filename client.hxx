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

        std::string name;

    private:
        void read_convo(int cid, std::vector<std::string>& str);

        Interface* interface;
        int client_fd;
        std::mutex mut;
        int uid;
        int cid;  // Current cid

        std::string tempbuf;
};