#include "defn.h"

class Interface;

class Client {
    public:
        Client(char* name, int fd);
        void send_message(char* buf, int size);
        void recieve();
        void start_interface();
        void set_client_fd(int fd);

        char* name;

    private:
        Interface* interface;
        int client_fd;
        pthread_mutex_t mutex;
        int namelen;
};