/* Common shared net code between server and client */
#include "defn.hxx"

class net {
    public:
        static int send_msg(int fd, p_header header, std::string buf);
        static int read_msg(int fd, p_header& header, std::string& buf);
        static int read_data(int fd, int size, std::string& data);
        static int read_data(int fd, int size, void* data);
        static int send_header(int fd, p_header header);
        static int read_header(int fd, p_header& header);
        static p_header build_header();   // Not implemented
};