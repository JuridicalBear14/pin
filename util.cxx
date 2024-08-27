#include "util.hxx"

/* Output error message */
void util::error(int code, std::string message) {
    // NOT IMPLEMENTED
}

/* Convert a p_header status to string form */
std::string util::status2str(int status) {
    // NOT IMPLEMENTED
    return std::string("not implemented");
}

/* Log an event to output */
void util::log(std::streambuf stream, std::string event) {

}

/* Prompt user input and retrieve the result */
void util::prompt(std::string message, std::string& buffer) {

}

/* Check a given input string for excluded characters (for names and keys), if one is found return the offending character (or 0 for success) */
char util::char_exclusion(std::string str) {
    // Key exclusion list
    std::vector<char> exlcusions(KEY_EXCLUSIONS);

    // Loop and check for errors
    for (char c : str) {
        // Check if excluded char, if so return c
        if ((c < KEY_LOWER_BOUND) || (c > KEY_UPPER_BOUND) || std::count(exlcusions.begin(), exlcusions.end(), c)) {
            return c;
        }
    }

    return 0;
}