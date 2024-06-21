#include "defn.h"

class Interface;

class Client {
    public:
        Client(std::string name, int fd);
        void send_message(int status, std::string buf);
        void recieve();
        void start_interface();
        void set_client_fd(int fd);

        std::string name;

    private:
        int read_message(p_header& header, std::string& str);

        Interface* interface;
        int client_fd;
        pthread_mutex_t mutex;
        int uid;
};