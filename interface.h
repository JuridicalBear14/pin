#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>

class Client;

#define NAMELEN 15     // Max name length
#define MAXMSG 1024    // Max message length
#define MSGGAP 0   // Gap between messages

// UI stuff
#define TYPEBOX_HEIGHT 2


/*
 * Abstract user interface class
*/
class Interface {
    private:
        Client* parent;
        pthread_mutex_t mutex;
};


class MessageWindow: Interface {

};

class ScrollableList: Interface {

};

class LoginScreen: Interface {

};