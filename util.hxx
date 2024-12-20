#pragma once

#include "defn.hxx"
/* Class of static utility functions for various uses */

// Status and error descriptors
const std::string STATUS_DESCRIPTORS[] = {
    "STATUS_NULL",
    "STATUS_CONNECT",
    "STATUS_MSG",
    "STATUS_MSG_OLD",   // Message being sent to catch up to db
    "STATUS_ITEM_COUNT",   // Message that defines a count of following messages, rather than a number of bytes
    "STATUS_DB_FETCH",   // Client requesting data from db (convo id)
    "STATUS_DB_SYNC",    // Sync db contents with client
    "STATUS_CONVO_CREATE",    // Create a new convo with name following
    "STATUS_CONNECT_DENIED",   // Denial of connection
    "STATUS_ERROR",   // Something on sender's end failed
    "STATUS_USER_AUTH",   // Authenticating user
    "STATUS_USER_DENIED",   // User auth denied
    "STATUS_NEW_USER",     // New user request
    "STATUS_DISCONNECT"
};

const std::string ERROR_DESCRIPTORS[] = {
    "E_BEGIN",

    "E_NO_SPACE",
    "E_NONE",
    "E_CONNECTION_CLOSED",
    "E_BAD_ADDRESS",
    "E_BAD_VALUE",
    "E_FAILED_READ",
    "E_FAILED_WRITE",
    "E_TOO_BIG",
    "E_GENERIC",
    "E_DENIED",
    "E_CONFLICT",
    "E_NOT_FOUND",

    "E_END"   // Final error code
};

class util {
    public:
        inline static char* logfile = nullptr;

        static void error(int code, std::string message);
        static std::string error2str(int code);
        static std::string status2str(int status);
        static char char_exclusion(std::string str);
        static std::vector<std::string> tokenize(std::string str);
        static void tolower(std::string& buf);
        template <typename T>
        inline static bool contains(std::vector<T>& list, const T& item) {return (bool) std::count(list.begin(), list.end(), item);}

        // Logging variants
        static void log(std::ostream& stream, std::string& message);   // Custom out
        static void log(std::string& message);   // Any message
        static void log(const char* message);
        static void log(int status, int uid, std::string uname, int cid);  // Message
        static void log(int id, std::string name, const char* message);  // User event
        static void log(int slot, int fd);  // Connection accept
        static void log(const char* message, int num);  // Message and number
        static void log(const char* message, std::string buf);

        // Serialization funcs
        static int serialize(char* buf, int size, p_header h);
        static int serialize(char* buf, int size, User user);
        static int serialize(char* buf, int size, Convo convo);

        // Deserialization funcs
        static int deserialize(char* buf, p_header& h);
        static int deserialize(char* buf, User& user);
        static int deserialize(char* buf, Convo& convo);

        // Real size funcs (for serialization)
        static inline int ssize(p_header h) { return (((sizeof(int) * 2) + NAMELEN + (KEYLEN * 2 ) + 3) + (sizeof(int) * 3)); }
        static inline int ssize(User u) { return (((sizeof(int) * 2) + NAMELEN + (KEYLEN * 2 ) + 3)); }
        static inline int ssize(Convo c) { return (sizeof(int) + sizeof(bool) + ((MAX_CONVO_USERS + 1) * (NAMELEN + 1))); }
};