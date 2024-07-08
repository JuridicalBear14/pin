/* Implementation for common net functions */
#include "net.hxx"

/* Read a message into str and return the header */
int net::read_msg(int fd, p_header& header, std::string& str) {
    // First read header
    int ret = read(fd, &header, sizeof(p_header));
    if (ret < sizeof(p_header)) {
        return 0;
    }

    int size = header.size % MAXMSG;    // % MAXMSG in case somehow bigger than max
    int bytes_read;

    // If something else to read, read it
    if (size > 0) {
        char buf[size + 1];   // +1 to allow space for null byte
        memset(buf, 0, sizeof(buf));

        bytes_read = read(fd, buf, size);
        str.assign(buf);
    }

    return size;
}

/* Send a message to a client */
int net::send_msg(int fd, p_header header, std::string buf) {
    // Make sure string isn't too big
    if (buf.size() > MAXMSG) {
        return -2;
    }

    // Send header
    int h_ret = send(fd, &header, sizeof(header), 0);

    // Check for success
    if (h_ret < sizeof(p_header)) {
        return -1;
    }

    // Send message
    int d_ret = send(fd, buf.c_str(), header.size, 0);

    return d_ret;
}

/* Send just header to remote */
int net::send_header(int fd, p_header header) {
    int ret = send(fd, &header, sizeof(header), 0);

    return ret;
}

/* Read just a header */
int net::read_header(int fd, p_header& header) {
    int ret = read(fd, &header, sizeof(header));

    return ret;
}

/* Build a header struct from given args and return it */
p_header net::build_header() {
    // NOT IMPLEMENTED
    p_header h;
    return h;
}