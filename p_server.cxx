#include "server.hxx"

void* start_server_relay(void* args) {
    Server* s = (Server*) args;
    s->start_server();

    return NULL;
}

/* Read server settings from config file */
int read_settings(struct server_settings& s) {
    // First make everything default settings
    s.db = DB_DEFAULT;
    s.port = DEFAULT_PORT;

    /*----------------------------------------*/

    // Open settings file
    std::string filename = SERVER_SETTINGS_FILE;
    std::ifstream f(filename);

    // Check if it didn't open correctly
    if (!f.good()) {
        return -1;
    }

    // Now loop through and set any settings found
    // Not sure if this is the most effective way to do it, but it works
    std::string buf;
    std::string set;
    std::string val;
    while (std::getline(f, buf)) {
        // If comment (starts with #) skip
        if (buf[0] == '#' || isspace(buf[0]) || (buf.find(' ') == std::string::npos)) {
            continue;
        }

        std::transform(buf.begin(), buf.end(), buf.begin(), ::tolower);

        // Otherwise switch case
        set = buf.substr(0, buf.find(' '));   // Grab first word, aka which setting
        val = buf.substr(buf.find(' ') + 1);

        if (val.size() < 1) {
            // No val, continue
            continue;
        }

        // Try catch for stoi
        try {
            if (set == "db") {
                s.db = std::stoi(buf.substr(buf.find(" ") + 1));
            } else if (set == "port") {
                s.port = std::stoi(buf.substr(buf.find(" ") + 1));
            }
        } catch (...) {
            std::cout << "Error parsing settings file on line (will use default instead):\n" << buf << "\n";
        }
    }

    return 0;
}

/* Main startup and port accept function */
int main(void) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Read server settings
    struct server_settings settings;
    if (read_settings(settings)) {
        std::cout << "Unable to read config file, using default settings\n";
    }

    std::cout << "Starting server...\n";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
       perror("setsockopt");
       exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(settings.port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAXUSR) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    Server s(server_fd);
    Database* db = new DB_FS(settings.db);

    // Connect db to server
    s.connect_db(db);

    // Set up relay thread
    std::thread r_thread(start_server_relay, &s);
    r_thread.detach();

    // Playing with printing IP
    /*
    char name[1024];
    gethostname(name, 1024);
    fprintf(stderr, "Server running, accepting connections:\n");
    fprintf(stderr, "IP address is: %s\n", name);
    fprintf(stderr, "port is: %d\n", (int) ntohs(address.sin_port));
    */

    s.connection_listener(address, addrlen);

    // Closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    delete db;
    return 0;
}
