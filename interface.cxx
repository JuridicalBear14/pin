#include "local.hxx"

// ********************************** Basic interface implementation *********************************************** //

void Interface::set_parent(Client* c) {
    parent = c;
}

/* Redraw the entire screen (used mostly for window resizing) */
int Interface::redraw_screen() {
    mutex.lock();

    // Clear existing window
    endwin();
    refresh();

    // Reset globals
    x = 0;
    y = 0;

    mutex.unlock();

    // Start new interface
    return create_screen();
}

/* End the screen control without throwing away any data */
void Interface::background() {
    mutex.lock();

    // Clear existing window
    endwin();
    refresh();

    // Reset cursor since we won't save typed text
    x = 0;
    y = 0;

    mutex.unlock();
}

/* Get my username from parent client */
std::string Interface::get_name() {
    return parent->name;
}

// ********************************** Message box interface implementation *********************************************** //

/* Write all data to screen */
void MessageWindow::write_to_screen() {
    mutex.lock();
    int line = MESSAGE_BOX_HEIGHT - 1;   // Line to add to, starts at bottom

    // Clear message_box
    clear_window(message_box, MESSAGE_BOX_HEIGHT);

    int line_height;
    for (int i = messages.size() - 1 - display_offset; i >= 0; i--) {
        // Get line height for wrapping pruposes
        line_height = messages[i].length() / MESSAGE_BOX_WIDTH;

        mvwaddstr(message_box, (line -= line_height), 0, messages[i].c_str());
        line -= 1 + MSGGAP;   // Allows for configurable text gap
    }

    wmove(typebox, y, x);
    keypad(typebox, TRUE);
    wrefresh(message_box);
    wrefresh(typebox);

    mutex.unlock();
}

/* Add message to list */
void MessageWindow::update_data(std::string buf, int type) {
    mutex.lock();

    // If somehow empty string, discard
    if (buf.length() < 1) {
        mutex.unlock();
        return;
    }
    
    switch (type) {
        case STATUS_NULL:   // Local
            messages.push_back("<" + get_name() + "> " + buf);
            break;

        case STATUS_MSG:   // New msg
            messages.push_back(buf);
            break;
        
        case STATUS_MSG_OLD:   // old msg
            //messages.insert(messages.begin(), buf);
            messages.push_back(buf);
            break;
    }

    mutex.unlock();
}


/* Clears a window's contents (no mutex b/c called under mutex) */
void MessageWindow::clear_window(WINDOW* win, int height) {
    // Create clearing string (C style because leagacy and it works)
    char blanks[MESSAGE_BOX_WIDTH];
    sprintf(blanks, "%*c", MESSAGE_BOX_WIDTH, ' ');

    // Clear each line
    for (int i = 0; i < height; i++) {
        mvwaddstr(win, i, 0, blanks);   // Clear line
    }

    wrefresh(win);
}

/* Create and draw a window */
WINDOW* MessageWindow::create_border(int height, int width, int x, int y) {
    WINDOW* temp;

    mutex.lock();

    temp = newwin(height, width, y, x);
    wborder(temp, '|', '|', '-', '-', '+', '+', '+', '+');

    wrefresh(temp);

    mutex.unlock();

    return temp;
}

/* Initial setup */
int MessageWindow::start_interface() {
    return create_screen();
}

/* Creates a new screen without modifying any underlying data */
int MessageWindow::create_screen() {
    // Set up gui stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();

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

    // Fetch messages from server
    std::vector<std::string> s;
    parent->fetch_convo(s);

    // Draw any messages
    write_to_screen();
    
    // Start event loop
    int exit_code = event_loop(typebox);

    endwin();
    //refresh();

    // Reset cursor since we won't save typed text
    x = 0;
    y = 0;

    return exit_code;
}

// Main event loop for keys
int MessageWindow::event_loop(WINDOW* typebox) {
    // A couple quick constants (not const bc screen resizing)
    int XMAX = TYPEBOX_WIDTH - 1;
    int YMAX = TYPEBOX_HEIGHT - 1;  // -1 because checks at end of line
    int XSTART = 0;

    int ch;    // int for expanded char set
    
    // Buffer to hold current message being typed
    std::string buffer;

    // Move to start of box
    wmove(typebox, y, x);
    wrefresh(typebox);

    // Run until F1 quit key
    while ((ch = wgetch(typebox)) != KEY_F(1)) {
        if (isprint(ch)) {  // If regular key, just write
            if (x < XMAX) {
                waddch(typebox, ch);
                buffer.push_back((char) ch);
                x++;

            } else {
                // At edge, new line
                if (y < YMAX) {
                    waddch(typebox, ch);
                    buffer.push_back((char) ch);
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
                        buffer.pop_back();
                    }
                    break;

                // Arrow keys
                case KEY_UP:
                    display_offset++;
                    write_to_screen();
                    break;
                case KEY_DOWN:
                    display_offset == 0 ? : display_offset--;
                    write_to_screen();
                    break;

                case '\n':   // Enter
                    // If not empty, add
                    if (buffer.length() > 0) {
                        update_data(buffer, STATUS_NULL);
                        write_to_screen();

                        parent->send_message(STATUS_MSG, buffer);

                        mutex.lock();
                        clear_window(typebox, TYPEBOX_HEIGHT);
                        buffer.clear();
                        mutex.unlock();

                        y = 0; x = 0;
                    }
                    break;

                case KEY_F(2):   // Screen refresh
                    return redraw_screen();
                case KEY_RESIZE:   // Screen resize
                    return redraw_screen();
                case KEY_F(3):
                case '\e':   // Escape
                    // Return control but don't exit the whole program
                    return EXIT_BG;
            }
        }


        wmove(typebox, y, x);
        wrefresh(typebox);
    }

    return EXIT_NONE;
}