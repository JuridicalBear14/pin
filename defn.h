/* Shared definitions between client and server */

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

// Struct for communication header
/*
struct p_header {
    uint64_t uid;    // User id
    u_int64_t cid;    // Convo id
    int type;   // Message type
    int status;   // Generic status int
    u_int64_t size;   // Size of following data (bytes)
}; */