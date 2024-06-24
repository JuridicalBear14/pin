/* Implementation for Databases */
#include "database.h"

// ********************************** Basic database implementation *********************************************** //



// ********************************** File system database implementation *********************************************** //

/* Constructor for file system database. Id determines which db to use (-1 for none, 0 for make new) */
DB_FS::DB_FS(int id) {
    if (id == DB_NONE) {  // None
        db_id = DB_NONE;
    } 

    // Build the database file system (or do nothing if it already exists), then return all indexed dbs
    std::vector<int> databases;
    build_FS(databases);
    
    if ((id == DB_NEW) || (id == DB_DEFAULT && databases.size() < 1)) {  // Create new
        db_id = build_db();

        if (db_id == DB_NONE) {
            std::cout << "Unable to create database, defaulting to none\n";
        }

        return;
    }

    // If default then use head
    if (id == DB_DEFAULT) {
        db_id = databases.back();
        db_path = "data/pin_db_" + std::to_string(databases[0]) + "/";

        std::cout << "Using default database: " << db_path << "\n";

        return;
    }

    // Otherwise fetch db
    if (std::find(databases.begin(), databases.end(), id) != databases.end()) {
        // It exists
        db_id = id;
        db_path = "data/pin_db_" + std::to_string(id) + "/";

        std::cout << "Using database at location: " << db_path << "\n";
    } else {
        // Doesn't exist
        std::cout << "Unable to find database, defaulting to none\n";
        db_id = DB_NONE;
    }
}

/* Build the pin file system structure and/or index all databases found in it */
void DB_FS::build_FS(std::vector<int>& entries) {
    // First check for data dir
    struct stat sb;
    if (stat(DATA_DIR, &sb)) {
        mkdir(DATA_DIR, 0777);
    }

    // Then open index file and read any contents
    std::ofstream create("data/index", std::ios::app); create.close();   // Create the file (if not already exists)
    std::ifstream f("data/index");

    std::string buf;
    while (std::getline(f, buf)) {
        if (!(buf.find_first_not_of("0123456789") == std::string::npos)) {   // Invalid entry (not numerical)
            continue; 
        }

        entries.push_back(std::stoi(buf));
    }

    f.close();
}

/* Build a new database and update index, return the id of the new db or DB_NONE for error */
int DB_FS::build_db() {
    // Find largest id
    int new_id = generate_id();

    return DB_NONE;
}

/* Generate a new unique db id */
int DB_FS::generate_id() {
    // NOT IMPLEMENTED
    return -1;
}

int DB_FS::add_user() {
    std::cout << "hello";
    return -1;
}

int DB_FS::add_msg(p_header header, std::string str) {
    return -1;
}

