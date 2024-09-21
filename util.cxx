#include "util.hxx"

/* Output error message */
void util::error(int code, std::string message) {
    std::cerr << "{ Error: " << error2str(code) << " | " << message << " }\n";
}

/* Convert an error code to a string */
std::string util::error2str(int code) {
    // Check if out of bounds
    if (code < E_NO_SPACE || code >= E_END) {  // No space is the lowest error code
        // Could be db error
        if (code == DB_ERR) {
            return "DB_ERR";
        }

        return "UNKOWN_ERROR_CODE";
    }

    return ERROR_DESCRIPTORS[code + 1];
}

/* Convert a p_header status to string form */
std::string util::status2str(int status) {
    // Check if out of bounds
    if (status < STATUS_NULL || status > STATUS_END) {
        return "UNKOWN_STATUS_CODE";
    }

    return STATUS_DESCRIPTORS[status];
}

/* Check a given input string for excluded characters (for names and keys), if one is found return the offending character (or 0 for success) */
char util::char_exclusion(std::string str) {
    // Key exclusion list
    std::vector<char> exclusions(KEY_EXCLUSIONS);

    // Loop and check for errors
    for (char c : str) {
        // Check if excluded char, if so return c
        if ((c < KEY_LOWER_BOUND) || (c > KEY_UPPER_BOUND) || std::count(exclusions.begin(), exclusions.end(), c)) {
            return c;
        }
    }

    return 0;
}

/* Tokenize an input string */
std::vector<std::string> util::tokenize(std::string str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    
    std::string buf;
    while (ss >> buf) {
        tokens.push_back(buf);
    }

    return tokens;
}

/* Convert to lowercase */
void util::tolower(std::string& buf) {
    std::transform(buf.begin(), buf.end(), buf.begin(), ::tolower);
}



/* Log an event to a given output without formatting */
void util::log(std::ostream& stream, std::string& message) {
    stream << message << "\n";
}

/* Write just a message to the default output */
void util::log(std::string& message) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    os << "| " << message << " |\n";
}

/* Write just a message to the default output (c string) */
void util::log(const char* message) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    os << "| " << message << " |\n";
}


/* Log a server message (this is magic number hell) */
void util::log(int status, int uid, std::string uname, int cid) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    char buf[1024];  // String to construct our message into
    std::snprintf(buf, sizeof(buf), "| Message recieved: %-21s | uid: %-2d | Username: %-15s | cid: %-2d |\n", status2str(status).c_str(), uid, uname.c_str(), cid);

    os << buf;
}

/* Log a user event */
void util::log(int id, std::string name, const char* message) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    char buf[1024];  // String to construct our message into
    std::snprintf(buf, sizeof(buf), "| Name: %-15s | id: %-2d | %s |\n", name.c_str(), id, message);

    os << buf;
}

/* Connection accept */
void util::log(int slot, int fd) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    os << "| Connection accepted | Slot: " << slot << " | fd: " << fd << " |\n";
}

/* Message and number */
void util::log(const char* message, int num) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    os << "| " << message << num << " |\n";
}

/* Message and string */
void util::log(const char* message, std::string buf) {
    std::ofstream _f;
    std::ostream& os = logfile ? (_f.open(logfile, std::ios::app), _f) : std::clog;

    os << "| " << message << buf << " |\n";
}