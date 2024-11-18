#include "util.hxx"

// MARK: <Misc>
// ****************************** <Misc> ************************* //

/* Output error message */
void util::error(int code, std::string message) {
    std::cerr << "{ Error: " << error2str(code) << " | " << message << " }\n";
}

/* Convert an error code to a string */
std::string util::error2str(int code) {
    code -= E_BEGIN;
    
    // Check if out of bounds
    if (code <= 1 || code >= (E_END - E_BEGIN)) {  // No space is the lowest error code
        return "UNKOWN_ERROR_CODE";
    }

    return ERROR_DESCRIPTORS[code];
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

// ****************************** </Misc> ************************* //





// MARK: Log
// ****************************** <Log> ************************* //

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

// ****************************** </Log> ****************************** //





// MARK: Serialize
// ****************************** <Serialize> ************************* //

/* Serialize p_header */
int util::serialize(char* buf, int size, p_header h) {
    // True size of p_header (without padding), since it's just a user and 3 ints
    if (size < ssize(h)) {
        return E_NO_SPACE;
    }

    // Tracking pointer
    char* ptr = buf;

    // First read user
    serialize(buf, size, h.user);
    ptr += ssize(h.user);  // Move by the size of user

    // Now do the ints
    int status = htonl(h.status);
    int data = htonl(h.data);
    int hsize = htonl(h.size);

    memcpy(ptr, &status, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, &data, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, &hsize, sizeof(int));
    ptr += sizeof(int);

    return E_NONE;
}

/* Serialize User */
int util::serialize(char* buf, int size, User user) {
    // True size of p_header (without padding)
    if (size < ssize(user)) {
        return E_NO_SPACE;
    }
    
    // Reverse byte order on ints
    int uid = htonl(user.uid);
    int cid = htonl(user.cid);

    // Pointer to move through buf's fields
    char* ptr = buf;

    // Copy ints
    memcpy(ptr, &uid, sizeof(int));
    ptr += sizeof(int);   // Now move pointer to next spot
    memcpy(ptr, &cid, sizeof(int));
    ptr += sizeof(int);

    // Copy strings
    memcpy(ptr, &(user.name), NAMELEN);   // Only move namelen, then set null manually to ensure safety
    ptr += NAMELEN;
    *(ptr++) = 0;

    memcpy(ptr, &(user.master_key), KEYLEN);   // Only move namelen, then set null manually to ensure safety
    ptr += KEYLEN;
    *(ptr++) = 0;

    memcpy(ptr, &(user.dynamic_key), KEYLEN);   // Only move namelen, then set null manually to ensure safety
    ptr += KEYLEN;
    *(ptr++) = 0;

    return E_NONE;
}

/* Serialize convo */
int util::serialize(char* buf, int size, Convo convo) {
    // True (unpadded) size
    if (size < ssize(convo)) {
        return E_NO_SPACE;
    }

    char* ptr = buf;

    int cid = htonl(convo.cid);

    // Copy fields
    memcpy(ptr, &cid, sizeof(int));
    ptr += sizeof(int);

    memcpy(ptr, &(convo.global), sizeof(bool));
    ptr += sizeof(bool);

    memcpy(ptr, convo.users, (MAX_CONVO_USERS * (NAMELEN + 1)));
    ptr += (MAX_CONVO_USERS * (NAMELEN + 1));

    memcpy(ptr, convo.name, (NAMELEN + 1));

    return E_NONE;
}

/* Deserialize p_header */
int util::deserialize(char* buf, p_header& h) {
    char* ptr = buf;

    // First user
    deserialize(ptr, h.user);
    ptr += ssize(h.user);

    // Now ints
    memcpy(&(h.status), ptr, sizeof(int));
    ptr += sizeof(int);
    h.status = htonl(h.status);   // Make sure to switch byte order

    memcpy(&(h.data), ptr, sizeof(int));
    ptr += sizeof(int);
    h.data = htonl(h.data);   // Make sure to switch byte order

    memcpy(&(h.size), ptr, sizeof(int));
    ptr += sizeof(int);
    h.size = htonl(h.size);   // Make sure to switch byte order

    return E_NONE;
}

/* Deserialize User */
int util::deserialize(char* buf, User& user) {
    char* ptr = buf;

    memcpy(&(user.uid), ptr, sizeof(int));
    ptr += sizeof(int);
    user.uid = htonl(user.uid);   // Make sure to switch byte order

    memcpy(&(user.cid), ptr, sizeof(int));
    ptr += sizeof(int);
    user.cid = htonl(user.cid);   // Make sure to switch byte order

    memcpy(&(user.name), ptr, (NAMELEN + 1));
    ptr += NAMELEN + 1;

    memcpy(&(user.master_key), ptr, (KEYLEN + 1));
    ptr += KEYLEN + 1;

    memcpy(&(user.dynamic_key), ptr, (KEYLEN + 1));

    return E_NONE;
}

/* Deserialize Convo */
int util::deserialize(char* buf, Convo& convo) {
    char* ptr = buf;

    memcpy(&(convo.cid), ptr, sizeof(int));
    ptr += sizeof(int);
    convo.cid = htonl(convo.cid);   // Make sure to switch byte order

    memcpy(&(convo.global), ptr, sizeof(bool));
    ptr += sizeof(bool);

    memcpy(&(convo.users), ptr, (NAMELEN + 1) * (MAX_CONVO_USERS));
    ptr += (NAMELEN + 1) * (MAX_CONVO_USERS);

    memcpy(&(convo.name), ptr, (NAMELEN + 1));

    return E_NONE;
}

// ****************************** </Serialize> ************************* //