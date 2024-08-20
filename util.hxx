#include "defn.hxx"
/* Class of static utility functions for various uses */

class util {
    public:
        static void error(int code, std::string message);
        static std::string status2str(int status);
        static void log(std::streambuf stream, std::string event);
        static void prompt(std::string message, std::string& buffer);
};