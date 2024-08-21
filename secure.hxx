#include "defn.hxx"
#include <termios.h>
#include <random>
/* Class for security focused utilities */

// Key character range bounds
#define KEY_LOWER_BOUND 48  // 0 (skips shift-number keys since they have special bash meanings)
#define KEY_UPPER_BOUND 126  // Tilde (end of typable characters)

// Table of character exclusions for key generation (since keys are otherwise comprised of all typable characters past 0)
#define KEY_EXCLUSIONS {'\\', '|', '`', '<', '>', ';'}
// ; < > \ ` |

class secure {
    public:
        static int encrypt(std::string& input);
        static int generate_key(char* buf);
        static int call_net(User user, int (*func), int count, ...);
        static bool validate_user(User user, User record);

        static void hide_keystrokes();
        static void show_keystrokes();
};