#include "secure.hxx"

/* Securely call a net function while verifying the validity of the user */
int secure::call_net(User user, int (*func), int count, ...) {
    return -1;
}

/* Lexically encrypt a given string */
int secure::encrypt(std::string input) {
    // NOT IMPLEMENTED
    return E_NONE;
}

/* Generate a random user key */
int secure::generate_key(char* buf) {
    if (buf == NULL) {
        return E_BAD_ADDRESS;
    }

    // Set up randomizer
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(33, 126);

    // Now loop and generate key
    int i = 0;
    for (; i < KEYLEN; i++) {
        buf[i] = dist(mt);
    }

    buf[i] = 0;
    return E_NONE;
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