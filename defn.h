/* Shared definitions between client and server */
#pragma once

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
#include <vector>
#include <thread>

#define PORT 5555
#define MAXUSR 10
#define NAMELEN 15     // Max name length
#define MAXMSG 1024    // Max message length

// Header constants
#define STATUS_NULL 0
#define STATUS_CONNECT 1
#define STATUS_MSG 2

// Struct for communication header
struct p_header {
    int uid;    // User id
    int cid;    // Convo id
    int status;   // Message type
    uint64_t size;   // Size of following data (bytes)
};