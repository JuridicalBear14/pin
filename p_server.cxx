#include "server.hxx"

// Yes, I am extremely lazy
struct sockaddr_in address;
int addrlen;

void* start_server_relay(void* args) {
    Server* s = (Server*) args;
    s->start_server();

    return NULL;
}

void* start_connection_listener(void* args) {
    Server* s = (Server*) args;
    s->connection_listener(address, addrlen);

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

        util::tolower(buf);

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
            util::log("Error parsing settings file on line (will use default instead): ", buf);
        }
    }

    return 0;
}

/* Main startup and port accept function */
int main(int argc, char** argv) {
    int server_fd, new_socket;
    int opt = 1;
    addrlen = sizeof(address);

    // Check for command line arg
    if (argc > 1) {
        util::logfile = argv[1];
    }

    // Read server settings
    struct server_settings settings;
    if (read_settings(settings)) {
        util::log("Unable to read config file, using default settings");
    }

    //std::cout << "Starting server...\n";
    util::log("Starting server...");

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
    Server_control scontrol(&s);
    Database* db = new DB_FS(settings.db);

    // Connect db to server
    s.connect_db(db);

    // Set up relay thread
    std::thread r_thread(start_server_relay, &s);
    std::thread cl_thread(start_connection_listener, &s);

    // Detach both since we only want to wait on server controls
    r_thread.detach();
    cl_thread.detach();

    scontrol.start_scontrol();

    // Playing with printing IP
    /*
    char name[1024];
    gethostname(name, 1024);
    fprintf(stderr, "Server running, accepting connections:\n");
    fprintf(stderr, "IP address is: %s\n", name);
    fprintf(stderr, "port is: %d\n", (int) ntohs(address.sin_port));
    */

    // Closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    delete db;
    return 0;
}
