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
#include <limits.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <algorithm>
#include <future>
#include <sstream>

#define PIN_VERSION 1


/* Configurable settings, with ifndef blocks so that they can be assigned in options header or when compiling */
#if __has_include("options.hxx")
#include "options.hxx"
#endif

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 55555
#endif

#ifndef MAXUSR
#define MAXUSR 10    // Max number of users on the network at one time
#endif

#ifndef NAMELEN
#define NAMELEN 15     // Max name length
#endif

#ifndef MAXMSG
#define MAXMSG 1024    // Max message length
#endif

#ifndef MAX_CONVO_USERS
#define MAX_CONVO_USERS 10  // Max number of users for one convo (other than all)
#endif

#ifndef KEYLEN
#define KEYLEN 6   // Length of a user authentication key
#endif

#ifndef TIMEOUT
#define TIMEOUT 3  // Amount of time (in seconds) to wait for a read before timing out
#endif

#ifndef SERVER_SETTINGS_FILE
#define SERVER_SETTINGS_FILE "pin.conf"
#endif

#ifndef DATA_DIR
#define DATA_DIR "data"   // The name of our database directory
#endif


// Server fd slot states
#define SERVER_SLOT_EMPTY -1
#define SERVER_SLOT_CLOSED -2

// Exit code stuff for interface and client
#define EXIT_FULL -1   // Exit program fully
#define EXIT_BG -2   // Put this interface in the background on exit
#define EXIT_COMPLETE -3  // Exit from completed task (for sub-interfaces)
#define EXIT_ERROR -4   // Exited with some kind of error

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
    STATUS_NEW_USER,     // New user request
    STATUS_DISCONNECT,   // Disconnect user
    STATUS_PING,   // Ping to check for server

    STATUS_END   // Final status code (for bounding purposes)
};

// Error codes
enum error_code {
    E_BEGIN = -1000,    // Set to min to make all codes negative
    
    E_NO_SPACE,
    E_NONE,
    E_CONNECTION_CLOSED,
    E_BAD_ADDRESS,
    E_BAD_VALUE,
    E_FAILED_READ,
    E_FAILED_WRITE,
    E_TOO_BIG,
    E_GENERIC,
    E_DENIED,
    E_CONFLICT,
    E_NOT_FOUND,

    E_END   // Final error code
};

// Key/username character range bounds
#define KEY_LOWER_BOUND 48  // 0 (skips shift-number keys since they have special bash meanings)
#define KEY_UPPER_BOUND 126  // ~ (end of typable characters)

// Table of character exclusions for key generation (since keys are otherwise comprised of all typable characters past 0)
#define KEY_EXCLUSIONS {'\\', '|', '`', '<', '>', ';'}
// ; < > \ ` |

// File type constants
#define FILE_TYPE_NULL 0
#define FILE_TYPE_CONVO_INDEX 1
#define FILE_TYPE_USER_INDEX 2
#define FILE_TYPE_DB_INDEX 3

/* Network structs */

// Struct for user data
typedef struct User {
    int uid;   // User id
    int cid;    // Current cid
    char name[NAMELEN + 1];  // Name  (+1 for null-term)
    char master_key[KEYLEN + 1];   // Key to always unlock account, field should only be used on db end
    char dynamic_key[KEYLEN + 1];   // Dynamic session keys, used by both sides and for future logins
} User;

// Struct for communication header
struct p_header {
    User user;
    int status;   // Message type
    int data;   // Small data field for various uses
    unsigned int size;   // Size of following data (bytes)
};

/* Struct for convo data */
typedef struct Convo {
    int cid;
    bool global;   // Whether or not this is open to everyone (if not then users tab gives permission)
    char users[MAX_CONVO_USERS][NAMELEN + 1];
    char name[NAMELEN + 1];   // +1 for null-term
} Convo;


/* Non-network structs */

// Struct for file headers
struct pin_db_header {
    char magic[4] = "PIN";
    int version = PIN_VERSION;
    int type;
    int itemsize;
    int itemno;
};

/* Struct for database index */
struct db_index_header {
    int id;

    // Settings for data storage
    int namelen;
    int keylen;
    int max_convo_users;
    int max_message_length;
}; 