#pragma once

#include "defn.hxx"
/* Class of static utility functions for various uses */

class util {
    public:
        static std::streambuf logstream;

        static void error(int code, std::string message);
        static std::string error2str(int code);
        static std::string status2str(int status);
        static void prompt(std::string message, std::string& buffer);
        static char char_exclusion(std::string str);

        // Logging variants
        static void log(std::streambuf stream, std::string message);
};