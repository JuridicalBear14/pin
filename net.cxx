/* Implementation for common net functions */
#include "net.hxx"
#include "util.hxx"

// MARK: Read
// ****************************** <Readers> ************************* //

/* Read a message into str and header in header, returns E_NONE for success */
int net::read_msg(int fd, p_header& header, std::string& str) {
    // First read header
    if (read_header(fd, header) != E_NONE) {
        return E_FAILED_READ;
    }

    int size = header.size;
    int bytes_read;

    // If something else to read, read it
    if (size > 0) {
        char buf[size + 1];   // +1 to allow space for null byte
        memset(buf, 0, sizeof(buf));

        bytes_read = read(fd, buf, size);

        if (bytes_read < size) {
            return E_FAILED_READ;
        }

        str.assign(buf);
    }

    return E_NONE;
}

/* Read just a header */
int net::read_header(int fd, p_header& header) {
    char buf[util::ssize(header)];
    int ret = read(fd, buf, sizeof(buf));

    if (ret == 0) {
        return E_CONNECTION_CLOSED;
    } else if (ret < (int) sizeof(buf)) {
        return E_FAILED_READ;
    }

    // Deserialize
    return util::deserialize(buf, header);
}

/* Read just the data of a message (when the header was already read) */
int net::read_data(int fd, int size, std::string& data) {
    if (size < 1) {
        return E_BAD_VALUE;
    } else if (size > MAXMSG) {
        return E_TOO_BIG;
    }

    char buf[size + 1];   // +1 to allow space for null byte
    memset(buf, 0, sizeof(buf));

    int bytes_read = read(fd, buf, size);

    if (bytes_read < size) {
        return E_FAILED_READ;
    }

    data.assign(buf);

    return E_NONE;
}

/* Read just the data of a message (when the header was already read) into a generic pointer */
int net::read_data(int fd, int size, void* data) {
    if (size < 1) {
        return E_BAD_VALUE;
    } else if (size > MAXMSG) {
        return E_TOO_BIG;
    }

    int bytes_read = read(fd, data, size);

    if (bytes_read < size) {
        return E_FAILED_READ;
    }

    return E_NONE;
}

// ****************************** </Readers> ************************* //





// MARK: Write
// ****************************** <Writers> ****************************** //

/* Send a message to a client */
int net::send_msg(int fd, p_header header, std::string buf) {
    // Make sure string isn't too big
    if (buf.size() > MAXMSG) {
        return E_TOO_BIG;
    }

    // Send header
    if (send_header(fd, header) != E_NONE) {
        return E_FAILED_WRITE;
    }

    // Send message
    int d_ret = send(fd, buf.c_str(), header.size, 0);

    if (d_ret < header.size) {
        return E_FAILED_WRITE;
    }

    return E_NONE;
}

/* Send a message to a client (generic) */
int net::send_msg(int fd, p_header header, void* buf) {
    // Make sure data isn't too big
    if (header.size > MAXMSG) {
        return E_TOO_BIG;
    }

    // Send header
    if (send_header(fd, header) != E_NONE) {
        return E_FAILED_WRITE;
    }

    if (buf == NULL) {
        return E_BAD_ADDRESS;
    }

    // Send message
    int d_ret = send(fd, buf, header.size, 0);

    if (d_ret < header.size) {
        return E_FAILED_WRITE;
    }

    return E_NONE;
}

/* Send just header to remote */
int net::send_header(int fd, p_header header) {
    // First, serialize
    char buf[util::ssize(header)];
    if (util::serialize(buf, sizeof(buf), header) != E_NONE) {
        return E_FAILED_WRITE;
    }

    int ret = send(fd, buf, sizeof(buf), 0);

    if (ret < util::ssize(header)) {
        return E_FAILED_WRITE;
    }
    
    return E_NONE;
}

// ****************************** </Writers> ****************************** //