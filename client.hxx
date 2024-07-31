#include "defn.hxx"
#include "net.hxx"

class Interface;

class Client {
    public:
        Client(int fd);
        void interface_handler();
        int send_message(int status, std::string buf);
        void recieve();
        void set_client_fd(int fd);
        int init(std::string name, std::string key);
        int request_convo(std::vector<std::string>& str);
        int fetch_convo_options(std::vector<Convo>& v);
        std::string getname();

    private:
        int request_new_convo(Convo c);
        int build_new_convo(Convo& c);
        int send_and_wait(p_header header, void* buf, std::condition_variable* waiter);

        Interface* interface;
        int client_fd;
        std::mutex mut;
        User user;
        
        // Vars to synchronizing convos
        std::vector<Convo> convo_vector;
        std::condition_variable convo_waiter;
};