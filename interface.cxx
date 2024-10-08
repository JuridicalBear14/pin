#include "local.hxx"

// MARK: BASE
// ****************************** <Basic interface implementation> ****************************** //

void Interface::set_parent(Client* c) {
    parent = c;
}

/* Redraw the entire screen (used mostly for window resizing) */
int Interface::redraw_screen() {
    mutex.lock();

    // Clear existing window
    endwin();
    //refresh();

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
    //refresh();

    // Reset cursor since we won't save typed text
    x = 0;
    y = 0;

    mutex.unlock();
}

/* Get my username from parent client */
std::string Interface::get_name() {
    return parent->getname();
}

// ****************************** </Basic interface implementation> ****************************** //





// MARK: Message Box
// ****************************** <Message box interface implementation> ****************************** //

/* Initial setup */
int MessageWindow::start_interface() {
    // Reset global vars in case it's being reused
    messages.clear();
    x = 0;
    y = 0;
    display_offset = 0;

    return create_screen();
}

/* Define ncurses color pairs */
void MessageWindow::define_colors() {
    init_pair(COLOR_IBAR, COLOR_BLACK, COLOR_WHITE);
}

/* Creates a new screen without modifying any underlying data */
int MessageWindow::create_screen() {
    // Set up gui stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();

    // Set up colors
    define_colors();

    // Draw main border
    message_box_border = create_border(LINES - TLINES - INFO_BAR_HEIGHT, COLS, 0, 0);

    // Draw typebar border
    typebox_border = create_border(TLINES, COLS, 0, LINES - TLINES);

    // Setup window sizes
    TYPEBOX_WIDTH = COLS - 4;   // -4 -> 2 border 2 padding

    MESSAGE_BOX_WIDTH = COLS - 4;   // -4 -> 2 border 2 padding
    MESSAGE_BOX_HEIGHT = LINES - TLINES - 2 - INFO_BAR_HEIGHT;  // -2 border

    // Create actual windows inside borders
    message_box = newwin(MESSAGE_BOX_HEIGHT, MESSAGE_BOX_WIDTH, 1, 2);
    typebox = newwin(TYPEBOX_HEIGHT, TYPEBOX_WIDTH, LINES - TLINES + 1, 2);
    info_bar = newwin(INFO_BAR_HEIGHT, COLS, LINES - TLINES - 1, 0);

    // Switch input to typebox
    keypad(typebox, TRUE);

    // Move cursor to typebar
    wmove(typebox, 0, 0);

    // Refresh everything
    wrefresh(typebox);

    // Now we're ready to go
    active = true;

    // Fetch messages from server if we don't have any
    if (messages.size() == 0) {
        std::vector<std::string> s;
        parent->request_convo(s);
    }

    // Don't forget to draw info bar
    draw_info_bar();

    // Draw any messages
    write_to_screen();

    // Start event loop
    int exit_code = event_loop(typebox);

    endwin();

    // Reset cursor since we won't save typed text
    x = 0;
    y = 0;

    return exit_code;
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

/* Write all data to screen */
void MessageWindow::write_to_screen() {
    // Check if we are ready to write
    if (!active) {
        return;
    }

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

/* Draw the contents of the info bar */
void MessageWindow::draw_info_bar() {
    clear_window(info_bar, INFO_BAR_HEIGHT);

    // Build string to write
    std::stringstream s;
    Convo c = parent->getconvo();

    s << "  User: " << parent->getname() << " | Convo: " << c.name << " | Global: " << (c.global ? "Yes" : "No") << " | " << CONTROLS;
    std::string b;

    // First, write a colored line to fill background
    wattron(info_bar, COLOR_PAIR(COLOR_IBAR));
    mvwhline(info_bar, 0, 0, ' ', COLS);

    // Now write text
    mvwaddstr(info_bar, 0, 0, s.str().c_str());
    wattroff(info_bar, COLOR_PAIR(COLOR_IBAR));

    wrefresh(info_bar);
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

/* Send a message to the parent (to be sent sent to the server) */
int MessageWindow::send_message(std::string buffer) {
    update_data(buffer, STATUS_NULL);
    return parent->send_message(STATUS_MSG, buffer);
}

/* Main event loop for keys */
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
    while (active && ((ch = wgetch(typebox)) != KEY_F(1))) {
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
                        if (send_message(buffer) != E_NONE) {
                            active = false;
                            return EXIT_ERROR;
                        }

                        write_to_screen();

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
                    return EXIT_COMPLETE;
            }
        }


        wmove(typebox, y, x);
        wrefresh(typebox);
    }

    return active ? EXIT_FULL : EXIT_COMPLETE;
}

// ****************************** </Message box interface implementation> ****************************** //





// MARK: Scrollable List
// ****************************** <Scrollable list interface implementation> ****************************** //

/* Initial setup */
int ScrollableList::start_interface(std::vector<Convo> options) {
    items = options;

    // Make sure these are 0 on every run
    selected = 0;
    page = 0;

    return create_screen();
}

/* Define ncurses color pairs */
void ScrollableList::define_colors() {
    init_pair(COLOR_IBAR, COLOR_BLACK, COLOR_WHITE);
}

/* Creates a new screen without modifying any underlying data */
int ScrollableList::create_screen() {
    // Set up gui stuff
    initscr();
    cbreak();
    noecho();
    start_color();

    // Set up colors
    define_colors();

    // Draw main border
    list_box_border = create_border(LINES - INFO_BAR_HEIGHT, COLS, 0, INFO_BAR_HEIGHT);
    wrefresh(list_box_border);

    // Setup window sizes
    LIST_BOX_WIDTH = COLS - 4;   // -4 -> 2 border 2 padding
    LIST_BOX_HEIGHT = LINES - 4 - INFO_BAR_HEIGHT;  // -2 border, -2 padding

    // Create actual windows inside borders
    list_box = newwin(LIST_BOX_HEIGHT, LIST_BOX_WIDTH, 2 + INFO_BAR_HEIGHT, 2);
    info_bar = newwin(INFO_BAR_HEIGHT, COLS, 0, 0);

    // Set up pages
    ITEMS_PER_PAGE = LIST_BOX_HEIGHT / (LIST_ITEM_GAP + LIST_ITEM_HEIGHT);
    ITEMS_PER_PAGE -= (LIST_BOX_HEIGHT % (LIST_ITEM_GAP + LIST_ITEM_HEIGHT) == 0) ? 1 : 0;   // Ensures we have at least one row of padding for selection border
    
    TOTAL_PAGES = items.size() / ITEMS_PER_PAGE;
    TOTAL_PAGES += (items.size() % ITEMS_PER_PAGE) > 0 ? 1 : 0;

    page = page >= TOTAL_PAGES ? 0 : page;   // If we resize down, reset page
    selected = ITEMS_PER_PAGE * page;   // Ensures selection is always on current page after resizing

    // Hide cursor
    curs_set(0);

    keypad(list_box, TRUE);
    wrefresh(list_box);

    // Now we're ready to go
    active = true;

    // Draw items
    write_to_screen();
    
    // Start event loop
    int exit_code = event_loop();

    // Show cursor
    curs_set(1);

    endwin();
    //refresh();


    return exit_code;
}

/* Clears a window's contents (no mutex b/c called under mutex) */
void ScrollableList::clear_window(WINDOW* win, int height) {
    // Create clearing string (C style because leagacy and it works)
    char blanks[LIST_BOX_WIDTH];
    sprintf(blanks, "%*c", LIST_BOX_WIDTH, ' ');

    // Clear each line
    for (int i = 0; i < height; i++) {
        mvwaddstr(win, i, 0, blanks);   // Clear line
    }

    wrefresh(win);
}

/* Create and draw a window */
WINDOW* ScrollableList::create_border(int height, int width, int x, int y) {
    WINDOW* temp;

    temp = newwin(height, width, y, x);
    wborder(temp, '|', '|', '-', '-', '+', '+', '+', '+');

    return temp;
}

/* Write all data to screen */
void ScrollableList::write_to_screen() {
    // Check if we are ready to write
    if (!active) {
        return;
    }

    int line = 1;   // Line to add to, starts at btop
    int inum = ITEMS_PER_PAGE * page;  // Which item number we're on
    int total_box_size = LIST_ITEM_HEIGHT + LIST_ITEM_GAP;  // Total size of one item, +2 for border

    // Clear message_box
    clear_window(list_box, LIST_BOX_HEIGHT);

    // Construct top and bottom box borders
    char borders[LIST_BOX_WIDTH + 1];
    memset(borders + 1, '~', LIST_BOX_WIDTH - 1);
    borders[0] = '+'; borders[LIST_BOX_WIDTH - 1] = '+'; borders[LIST_BOX_WIDTH] = 0;

    char str[1024];
    for (int i = 0; (inum < items.size()) && (i < ITEMS_PER_PAGE); i++) {
        memset(str, 0, 1024);

        // If selected: box, otherwise plain
        if (inum == selected) {
            snprintf(str, 1024, "| %-*s |", LIST_BOX_WIDTH - 4, items[inum].name);

            mvwaddstr(list_box, line - 1, 0, borders);
            mvwaddstr(list_box, line, 0, str);
            mvwaddstr(list_box, line + 1, 0, borders);
        } else {
            snprintf(str, 1024, "[%d] %s", i + 1, items[inum].name);
            mvwaddstr(list_box, line, 0, str);
        }
        
        line += total_box_size;   // Allows for configurable item gap
        inum++;
    }

    wrefresh(list_box);

    // Don't forget to draw info bar
    draw_info_bar();
}

/* Draw the contents of the info bar */
void ScrollableList::draw_info_bar() {
    clear_window(info_bar, INFO_BAR_HEIGHT);

    // Build string to write
    std::stringstream s;

    s << "  User: " << parent->getname() << " | Key: " << parent->getkey() << " | Page: " << page + 1 << " of " << TOTAL_PAGES << " | Press 0 for new convo | " << CONTROLS;
    std::string b;

    // First, write a colored line to fill background
    wattron(info_bar, COLOR_PAIR(COLOR_IBAR));
    mvwhline(info_bar, 0, 0, ' ', COLS);

    // Now write text
    mvwaddstr(info_bar, 0, 0, s.str().c_str());
    wattroff(info_bar, COLOR_PAIR(COLOR_IBAR));

    wrefresh(info_bar);
}

/* Main event loop for keys */
int ScrollableList::event_loop() {
    int ch;    // int for expanded char set

    // Run until F1 quit key
    while ((ch = wgetch(list_box)) != KEY_F(1)) {
        if (isdigit(ch)) {  // If number key, select
            // If 0 always return 0
            if (ch == '0') {
                return 0;
            }

            if ((ch - '0') <= items.size()) {
                return (ch - '0') + (ITEMS_PER_PAGE * page);
            }

        } else {
            // Specific special case keys
            switch(ch) {
                case 'n':   // New convo
                    return 0;
                // Arrow keys
                case KEY_UP:
                    selected == 0 ? : selected--;
                    selected % ITEMS_PER_PAGE == ITEMS_PER_PAGE - 1 ? page = selected / ITEMS_PER_PAGE : selected;

                    write_to_screen();
                    break;
                case KEY_DOWN:
                    selected < items.size() - 1 ? selected++ : selected;

                    selected % ITEMS_PER_PAGE == 0 ? page = selected / ITEMS_PER_PAGE : selected;
                    write_to_screen();
                    break;

                case KEY_PPAGE:
                    (page <= 0) ? : selected = (--page) * ITEMS_PER_PAGE;
                    write_to_screen();
                    break;

                case KEY_NPAGE:
                    (page >= TOTAL_PAGES - 1) ? : selected = (++page) * ITEMS_PER_PAGE;
                    write_to_screen();
                    break;

                case '\n':   // Enter
                    return selected + 1;
                    break;

                case KEY_F(2):   // Screen refresh
                    return redraw_screen();
                case KEY_RESIZE:   // Screen resize
                    return redraw_screen();
                case KEY_F(3):
                case '\e':   // Escape
                    // Return control but don't exit the whole program
                    return EXIT_FULL;
            }
        }
    }

    return EXIT_FULL;
}

// ****************************** </Scrollable list interface implementation> ****************************** //





// MARK: Input Window
// ****************************** <Input window implementation> ****************************** //

/* Start the interface */
int InputWindow::start_interface(std::vector<std::string> prompts_initial, std::vector<std::string>& responses_final) {
    if (prompts_initial.size() < 1) {
        return E_BAD_VALUE;
    }

    prompts = prompts_initial;

    // Insert first prompt
    messages.push_back(prompts[0]);

    int ret = create_screen();

    // Set return val to collected responses
    responses_final = responses;

    return ret;
}

/* Draw info bar override to be more accurate */
void InputWindow::draw_info_bar() {
    clear_window(info_bar, INFO_BAR_HEIGHT);

    // First, write a colored line to fill background
    wattron(info_bar, COLOR_PAIR(COLOR_IBAR));
    mvwhline(info_bar, 0, 0, ' ', COLS);

    // Now write text
    mvwaddstr(info_bar, 0, 0, "  Creating new convo | Press [F3] to exit and save these users (adding no users will result in a global convo)");
    wattroff(info_bar, COLOR_PAIR(COLOR_IBAR));

    wrefresh(info_bar);
}

/* Message send rewrite (to instead save the value) */
int InputWindow::send_message(std::string buffer) {
    // First check if the buffer is valid
    char c;
    if ((c = util::char_exclusion(buffer)) != 0) {
        update_data(std::string("Error, invalid character: ") + c, STATUS_MSG);
        update_data(prompts[responses.size()], STATUS_MSG);

        return E_NONE;
    }
    
    // Save the buffer
    responses.push_back(buffer);

    // If we got all the responses, exit
    if (responses.size() >= prompts.size()) {
        active = false;
        return E_NONE;
    }

    update_data(buffer, STATUS_NULL);

    // Otherwise, insert next prompt and continue
    update_data(prompts[responses.size()], STATUS_MSG);

    return E_NONE;
}

// ****************************** </Input window implementation> ****************************** //