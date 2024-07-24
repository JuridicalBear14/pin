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
        void request_convo(std::vector<std::string>& str);
        int fetch_convo_options(std::vector<Convo>& v);
        std::string getname();

    private:
        int request_new_convo(Convo c);
        int build_new_convo(Convo& c);
        void send_and_wait(p_header header, void* buf, std::condition_variable* waiter);

        Interface* interface;
        int client_fd;
        std::mutex mut;
        User user;
        
        // Vars to synchronizing convos
        std::vector<Convo> convo_vector;
        std::condition_variable convo_waiter;
};