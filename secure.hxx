#pragma once

#include "defn.hxx"
#include <termios.h>
#include <random>
/* Class for security focused utilities */

// default encryption algorithm used
#define ENCRYPTION_ALG pcipher2

class secure {
    public:
        static int encrypt(char* input, int size);
        static int generate_key(char* buf);
        static int call_net(User user, int (*func), int count, ...);
        static bool validate_user(User user, User record);

        static void hide_keystrokes();
        static void show_keystrokes();
    
    private:
        // Encryption algs
        static int pcipher2(char* input, int size);
};