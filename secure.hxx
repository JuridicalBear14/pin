#include "defn.hxx"
#include <termios.h>
#include <random>
/* Class for security focused utilities */

class secure {
    public:
        static int encrypt(std::string input);
        static int generate_key(char* buf);
        static int call_net(User user, int (*func), int count, ...);
        static bool validate_user(User user, User record);

        static void hide_keystrokes();
        static void show_keystrokes();
};