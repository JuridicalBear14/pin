#include "local.h"
#include "defn.h"

// ********************************** Basic interface implementation *********************************************** //

void Interface::set_parent(Client* c) {
    parent = c;
}

/* Redraw the entire screen (used mostly for window resizing) */
void Interface::redraw_screen() {
    // Clear existing window
    endwin();
    refresh();

    // Reset globals
    x = 0;
    y = 0;

    // Start new interface
    create_screen();
}

/* Get my username from parent client */
char* Interface::get_name() {
    return parent->name;
}

// ********************************** Message box interface implementation *********************************************** //

/* Write all data to screen */
void MessageWindow::write_to_screen() {
    int line = MESSAGE_BOX_HEIGHT - 1;   // Line to add to, starts at bottom

    // Clear message_box
    clear_window(message_box, MESSAGE_BOX_HEIGHT);

    int line_height;
    for (int i = msgix - 1 - display_offset; i >= 0; i--) {
        // Get line height for wrapping pruposes
        line_height = (strlen(messages[i])) / MESSAGE_BOX_WIDTH;

        mvwaddstr(message_box, (line -= line_height), 0, messages[i]);
        line -= 1 + MSGGAP;   // Allows for configurable text gap
    }

    wmove(typebox, y, x);
    keypad(typebox, TRUE);
    wrefresh(message_box);
    wrefresh(typebox);
}

/* Add message to list */
void MessageWindow::update_data(char* buf, int len) {
    // Check if starts with '<' to see if local or remote
    if (buf[0] == '<') {   // Remote
        messages[msgix] = (char*) malloc(len);
        strncpy(messages[msgix], buf, strlen(buf));
    } else {   // Local
        char* name = get_name();
        int namelen = strlen(name);
        messages[msgix] = (char*) malloc(len + namelen + 4);   // +4 for <>[space] and null
        snprintf(messages[msgix], len + namelen + 4, "<%s> %s", name, buf);
    }
    
    // If limit hit, realloc
    if (++msgix == MSG_MAX) {
        messages = (char**) realloc(messages, MSG_MAX);
    }
}


// Clears a window's contents
void MessageWindow::clear_window(WINDOW* win, int height) {
    // Create clearing string
    char blanks[MESSAGE_BOX_WIDTH];
    sprintf(blanks, "%*c", MESSAGE_BOX_WIDTH, ' ');

    // Clear each line
    for (int i = 0; i < height; i++) {
        mvwaddstr(win, i, 0, blanks);   // Clear line
    }

    wrefresh(win);
}

// Create and draw a window
WINDOW* MessageWindow::create_border(int height, int width, int x, int y) {
    WINDOW* temp;

    temp = newwin(height, width, y, x);
    wborder(temp, '|', '|', '-', '-', '+', '+', '+', '+');

    wrefresh(temp);

    return temp;
}

// Initial setup
void MessageWindow::start_interface() {
    // Set up message array
    messages = (char**) calloc(MSG_MAX, sizeof(char*));

    // Create window
    create_screen();
}

// Creates a new screen without modifying any underlying data
void MessageWindow::create_screen() {
    // Set up gui stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    
    // Set up mutex
    pthread_mutex_init(&mutex, NULL);

    // Set up color pairs

    // Draw main border
    message_box_border = create_border(LINES - TLINES, COLS, 0, 0);

    // Draw typebar border
    typebox_border = create_border(TLINES, COLS, 0, LINES - TLINES);

    // Setup window sizes
    TYPEBOX_WIDTH = COLS - 4;   // -4 -> 2 border 2 padding

    MESSAGE_BOX_WIDTH = COLS - 4;   // -4 -> 2 border 2 padding
    MESSAGE_BOX_HEIGHT = LINES - TLINES - 2;  // -2 border

    // Create actual windows inside borders
    message_box = newwin(MESSAGE_BOX_HEIGHT, MESSAGE_BOX_WIDTH, 1, 2);
    typebox = newwin(TYPEBOX_HEIGHT, TYPEBOX_WIDTH, LINES - TLINES + 1, 2);

    // Switch input to typebox
    keypad(typebox, TRUE);

    // Move cursor to typebar
    wmove(typebox, 0, 0);

    // Refresh everything
    wrefresh(typebox);
    
    // Draw any messages
    write_to_screen();

    // Start event loop
    event_loop(typebox);

    endwin();
}

// Main event loop for keys
void MessageWindow::event_loop(WINDOW* typebox) {
    // A couple quick constants (not const bc screen resizing)
    int XMAX = TYPEBOX_WIDTH - 1;
    int YMAX = TYPEBOX_HEIGHT - 1;  // -1 because checks at end of line
    int XSTART = 0;

    int ch;    // int for expanded char set
    
    // Buffer to hold current message being typed
    char buffer[MAXMSG];
    int addix = -1;   // Next index to add character

    // Move to start of box
    wmove(typebox, y, x);
    wrefresh(typebox);

    // Run until F1 quit key
    while ((ch = wgetch(typebox)) != KEY_F(1)) {
        if (isprint(ch) && (addix < MAXMSG)) {  // If regular key, just write
            if (x < XMAX) {
                waddch(typebox, ch);
                buffer[++addix] = ch;
                x++;

            } else {
                // At edge, new line
                if (y < YMAX) {
                    waddch(typebox, ch);
                    buffer[++addix] = ch;
                    y++;
                    x = 0;
                }
            }

        } else {
            // Specific special case keys

            switch(ch) {
                case KEY_BACKSPACE:
                    if (!(x == 0 && y == 0)) {   // If not first char
                        if (x == 0 && y != 0) {  // If end of line
                            mvwaddch(typebox, --y, (x = XMAX), ' ');

                        } else {   // Normal space
                            mvwaddch(typebox, y, --x, ' ');
                        }

                        // Remove from buffer
                        buffer[addix--] = 0;
                    }
                    break;

                // Arrow keys
                case KEY_UP:
                    display_offset++;

                    pthread_mutex_lock(&mutex);
                    write_to_screen();
                    pthread_mutex_unlock(&mutex);

                    break;
                case KEY_DOWN:
                    display_offset == 0 ? : display_offset--;

                    pthread_mutex_lock(&mutex);
                    write_to_screen();
                    pthread_mutex_unlock(&mutex);

                    break;

                case '\n':   // Enter
                    // If not empty, add
                    if (strlen(buffer) > 0) {
                        buffer[++addix] = 0;

                        pthread_mutex_lock(&mutex);
                        update_data(buffer, addix + 1);
                        write_to_screen();
                        pthread_mutex_unlock(&mutex);

                        parent->send_message(buffer, addix + 1);

                        clear_window(typebox, TYPEBOX_HEIGHT);
                        memset(buffer, 0, addix);
                        addix = -1;

                        y = 0; x = 0;
                    }
                    break;

                case KEY_F(2):   // Screen refresh
                    redraw_screen();
                    return;
                case KEY_RESIZE:   // Screen resize
                    redraw_screen();
                    return;
            }
        }


        wmove(typebox, y, x);
        wrefresh(typebox);
    }
}