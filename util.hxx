#include "defn.hxx"
/* Class of static utility functions */

class util {
    public:
        static void error(int code, std::string message);
        static std::string status2str(int status);
};