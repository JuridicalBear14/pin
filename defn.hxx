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
#define MAXUSR 10
#define NAMELEN 15     // Max name length
#define MAXMSG 1024    // Max message length
#define MAX_CONVO_USERS 10  // Max number of users for one convo (other than all)

// Exit code stuff for interface and client
#define EXIT_NONE 0   // Exit program fully
#define EXIT_BG 1   // Put this interface in the background on exit

// Header constants
#define STATUS_NULL 0
#define STATUS_CONNECT 1
#define STATUS_MSG 2
#define STATUS_MSG_OLD 3   // Message being sent to catch up to db
#define STATUS_ITEM_COUNT 4   // Message that defines a count of following messages, rather than a number of bytes
#define STATUS_DB_FETCH 5   // Client requesting data from db (convo id)
#define STATUS_DB_SYNC 6   // Sync db contents with client
#define STATUS_CONVO_CREATE 7   // Create a new convo with name following

// File type constants
#define FILE_TYPE_NULL 0
#define FILE_TYPE_CONVO_INDEX 1

// Struct for communication header
struct p_header {
    int uid;    // User id
    int cid;    // Convo id
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

// Struct for user data
typedef struct User {
    int uid;   // User id
    int cid;    // Current cid
    char name[NAMELEN + 1];  // Name  (+1 for null-term)
} User;


/* Struct for convo data */
typedef struct Convo {
    int cid;
    int users[MAX_CONVO_USERS];
    char name[NAMELEN + 1];   // +1 for null-term
} Convo;