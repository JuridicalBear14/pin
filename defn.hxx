/* Shared definitions between client and server */
#pragma once

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <poll.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <algorithm>
#include <future>

#define PIN_VERSION 1

#define DEFAULT_PORT 5555
#define MAXUSR 10    // Max number of users on the network
#define NAMELEN 15     // Max name length
#define MAXMSG 1024    // Max message length
#define MAX_CONVO_USERS 10  // Max number of users for one convo (other than all)
#define KEYLEN 6   // Length of a user authentication key

// Exit code stuff for interface and client
#define EXIT_NONE 0   // Exit program fully
#define EXIT_BG 1   // Put this interface in the background on exit

// Header status codes
enum header_status {
    STATUS_NULL,
    STATUS_CONNECT,
    STATUS_MSG,
    STATUS_MSG_OLD,   // Message being sent to catch up to db
    STATUS_ITEM_COUNT,   // Message that defines a count of following messages, rather than a number of bytes
    STATUS_DB_FETCH,   // Client requesting data from db (convo id)
    STATUS_DB_SYNC,    // Sync db contents with client
    STATUS_CONVO_CREATE,    // Create a new convo with name following
    STATUS_CONNECT_DENIED,   // Denial of connection
    STATUS_ERROR,   // Something on sender's end failed
    STATUS_USER_AUTH,   // Authenticating user
    STATUS_USER_DENIED,   // User auth denied
    STATUS_NEW_USER     // New user request
};

// Error codes
enum error_code {
    E_NO_SPACE = -1,   // -1 to not conflict with other id systems
    E_NONE,
    E_CONNECTION_CLOSED,
    E_BAD_ADDRESS,
    E_BAD_VALUE,
    E_FAILED_READ,
    E_FAILED_WRITE,
    E_TOO_BIG,
    E_GENERIC
};

// File type constants
#define FILE_TYPE_NULL 0
#define FILE_TYPE_CONVO_INDEX 1

// Struct for user data
typedef struct User {
    int uid;   // User id
    int cid;    // Current cid
    char name[NAMELEN + 1];  // Name  (+1 for null-term)
    char key[KEYLEN + 1];
} User;

// Struct for communication header
struct p_header {
    User user;
    int status;   // Message type
    int data;   // Small data field for various uses
    uint64_t size;   // Size of following data (bytes)
};


// Struct for file headers
struct pin_db_header {
    char magic[4] = "PIN";
    int version = PIN_VERSION;
    int type;
    int itemsize;
    int itemno;
};


/* Struct for convo data */
typedef struct Convo {
    int cid;
    bool global;   // Whether or not this is open to everyone (if not then users tab gives permission)
    User users[MAX_CONVO_USERS];
    char name[NAMELEN + 1];   // +1 for null-term
} Convo;