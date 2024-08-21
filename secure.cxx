#include "secure.hxx"

/* Securely call a net function while verifying the validity of the user */
int secure::call_net(User user, int (*func), int count, ...) {
    return -1;
}

/* Lexically encrypt a given string */
int secure::encrypt(std::string& input) {
    // NOT IMPLEMENTED
    return E_NONE;
}

/* Generate a random user key */
int secure::generate_key(char* buf) {
    if (buf == NULL) {
        return E_BAD_ADDRESS;
    }

    // Key exclusion list
    std::vector<char> exlcusions(KEY_EXCLUSIONS);

    // Set up randomizer
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(KEY_LOWER_BOUND, KEY_UPPER_BOUND);   // Numbers are the ascii range for typable characters

    // Now loop and generate key
    int i = 0;
    int n = dist(mt);
    for (; i < KEYLEN; i++) {
        // Check if excluded char, repeat until not
        while (std::count(exlcusions.begin(), exlcusions.end(), n)) {
            n = dist(mt);
        }

        buf[i] = n;
        n = dist(mt);
    }

    buf[i] = 0;
    return E_NONE;
}

/* Validate key match for a perspective user and their record */
bool secure::validate_user(User user, User record) {
    // First we should check the name and uid
    if (strncmp(user.name, record.name, sizeof(user.name))) {
        // If different name/uid, wrong person
        return false;
    }

    // Now check the key(s) (only one has to match)
    if (strncmp(user.dynamic_key, record.master_key, sizeof(user.dynamic_key)) && strncmp(user.dynamic_key, record.dynamic_key, sizeof(user.dynamic_key))) {
        // Different key, meaning denied login
        return false;
    }

    // If keys are good, return true
    return true;
}

/* Functions to show and hide terminal keystrokes, credit to Nik Bougalis on Stack Overflow */
void secure::hide_keystrokes() {
    termios tty;

    tcgetattr(STDIN_FILENO, &tty);

    /* we want to disable echo */
    tty.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void secure::show_keystrokes() {
   termios tty;

    tcgetattr(STDIN_FILENO, &tty);

    /* we want to reenable echo */
    tty.c_lflag |= ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}