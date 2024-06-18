#include "defn.h"

class Interface;

class Client {
    public:
        Client(std::string name, int fd);
        void send_message(std::string buf);
        void recieve();
        void start_interface();
        void set_client_fd(int fd);

        std::string name;

    private:
        Interface* interface;
        int client_fd;
        pthread_mutex_t mutex;
};