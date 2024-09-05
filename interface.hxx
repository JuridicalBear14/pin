#pragma once

#include <ncurses.h>
#include <ctype.h>
#include "defn.hxx"

// UI stuff
#define TYPEBOX_HEIGHT 2
#define MSGGAP 0   // Gap between messages
#define INFO_BAR_HEIGHT 1
#define LIST_ITEM_HEIGHT 1
#define LIST_ITEM_GAP 1

// Popup stuff
#define POPUP_SWITCH 1
#define POPUP_TBOX 2

// Colors
#define COLOR_BG 1
#define COLOR_BORDER 2
#define COLOR_IBAR 3
#define COLOR_MSG 4

#define CONTROLS "[F1] Quit | [F2] Refresh | [F3] Return"    // User controls (to put in info bar)

class Client;

/*
 * Abstract user interface class
*/
class Interface {
    public:
        virtual int start_interface() {return -1;};
        virtual void write_to_screen() {};
        virtual void update_data(std::string buf, int type) {};
        void set_parent(Client* c);
        void background();

    protected:
        int redraw_screen();
        std::string get_name();
        virtual int create_screen() {return -1;};
        virtual int event_loop(WINDOW* window) {return -1;};
        virtual void define_colors() {};

        Client* parent;
        std::mutex mutex;
        int x = 0;   // Cursor positions
        int y = 0;
        bool active = false;
};


class MessageWindow: public Interface {
    public:
        // Overrides
        virtual int start_interface();
        void write_to_screen();
        void update_data(std::string buf, int type);

    protected:
        // Overrides
        int event_loop(WINDOW* typebox);
        int create_screen();
        void define_colors();
        virtual void draw_info_bar();
        virtual int send_message(std::string buffer);

        WINDOW* create_border(int height, int width, int x, int y);
        void clear_window(WINDOW* win, int height);

        // Windows
        WINDOW* message_box;
        WINDOW* typebox;
        WINDOW* info_bar;

        // Borders
        WINDOW* message_box_border;
        WINDOW* typebox_border;

        // Psuedo-constant window sizes
        int TLINES = TYPEBOX_HEIGHT + 2;
        int TYPEBOX_WIDTH;
        int MESSAGE_BOX_HEIGHT;
        int MESSAGE_BOX_WIDTH;

        // Global vars
        std::vector<std::string> messages;
        int display_offset = 0;   // Offset for which messages are displayed
        // (ex: 1 -> display all messages except most recent)
        bool running = true;   // Trigger to instantly stop the main even loop
};

class ScrollableList: public Interface {
    public:
        // Overrides
        int start_interface(std::vector<Convo> options);
        void write_to_screen();

    private:
        // Overrides
        int event_loop();
        int create_screen();
        void define_colors();
        void draw_info_bar();

        WINDOW* create_border(int height, int width, int x, int y);
        void clear_window(WINDOW* win, int height);

        // Windows
        WINDOW* list_box;
        WINDOW* info_bar;

        // Borders
        WINDOW* list_box_border;

        // Psuedo-constant window sizes
        int LIST_BOX_HEIGHT;
        int LIST_BOX_WIDTH;

        int TOTAL_PAGES = 0;
        int ITEMS_PER_PAGE = 0;

        // Global vars
        std::vector<Convo> items;
        int selected = 0;  // Which item we have selected currently
        int page = 0;   // Which page we're on
};
 
class LoginScreen: Interface {

};

class InputWindow : public MessageWindow {
    public:
        // Overrides
        int start_interface(std::vector<std::string> prompts, std::vector<std::string>& responses);

    private:
        int send_message(std::string buffer);
        void draw_info_bar();

        // Global vars
        std::vector<std::string> prompts;
        std::vector<std::string> responses;
};