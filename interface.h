#include <ncurses.h>
#include <ctype.h>
#include "defn.h"

// UI stuff
#define TYPEBOX_HEIGHT 2
#define MSGGAP 0   // Gap between messages

class Client;

/*
 * Abstract user interface class
*/
class Interface {
    public:
        virtual void start_interface() {};
        virtual void write_to_screen() {};
        virtual void update_data(char* buf, int len) {};
        void set_parent(Client* c);

    protected:
        void redraw_screen();
        char* get_name();
        virtual void create_screen() {};
        virtual void event_loop(WINDOW* window) {};

        // Colors

        Client* parent;
        pthread_mutex_t mutex;
        int x = 0;   // Cursor positions
        int y = 0;
};


class MessageWindow: public Interface {
    public:
        // Overrides
        void start_interface();
        void write_to_screen();
        void update_data(char* buf, int len);

    private:
        // Overrides
        void event_loop(WINDOW* typebox);
        void create_screen();

        WINDOW* create_border(int height, int width, int x, int y);
        void clear_window(WINDOW* win, int height);

        // Windows
        WINDOW* message_box;
        WINDOW* typebox;

        // Borders
        WINDOW* message_box_border;
        WINDOW* typebox_border;

        // Psuedo-constant window sizes
        int TLINES = TYPEBOX_HEIGHT + 2;
        int TYPEBOX_WIDTH;
        int MESSAGE_BOX_HEIGHT;
        int MESSAGE_BOX_WIDTH;

        int MSG_MAX = 1000;   // Initial max for message count, doubles when space runs out

        // Global vars
        char** messages;
        int msgix = 0;   // Current index in message array
        int display_offset = 0;   // Offset for which messages are displayed
        // (ex: 1 -> display all messages except most recent)
};

class ScrollableList: Interface {

};

class LoginScreen: Interface {

};