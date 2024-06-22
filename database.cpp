/* Implementation for Databases */
#include "database.h"

// ********************************** Basic database implementation *********************************************** //



// ********************************** File system database implementation *********************************************** //

/* Constructor for file system database. Id determines which db to use (-1 for none, 0 for make new) */
DB_FS::DB_FS(int id) {
    if (id == -1) {  // None

    } else if (id == 0) {  // Create new
        build_db();
    }

    // Otherwise fetch db
}

int DB_FS::add_user() {
    std::cout << "hello";
    return -1;
}

int DB_FS::add_msg(p_header header, std::string str) {
    return -1;
}

/* Build a database for this system */
int DB_FS::build_db() {
    // First check for data dir
    struct stat sb;
    if (stat(DATA_DIR, &sb)) {
        mkdir(DATA_DIR, 0777);
    }
    
    std::ofstream f("data/test.txt");
    f << "Hello world\n" << std::endl;
    return -1;
}