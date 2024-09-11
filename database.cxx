#include "database.hxx"

// MARK: BASE
// ****************************** <Database construction> ************************* //
// For now only file system db, theoretically could substitute for SQL

/* Constructor for file system database. Id determines which db to use (-1 for none, 0 for make new) */
DB_FS::DB_FS(int id) {
    if (id == DB_NONE) {  // None
        db_id = DB_NONE;
        return;
    } 

    int err;

    // Build the database file system (or do nothing if it already exists), then return all indexed dbs
    std::vector<int> databases;
    if ((err = build_FS(databases)) != E_NONE) {
        util::error(err, "Error: failed to build file system, defaulting to none");
        db_id = DB_NONE;
        return;
    }
    
    if ((id == DB_NEW) || (id == DB_DEFAULT && databases.size() < 1)) {  // Create new
        db_id = build_db();

        if (db_id == DB_ERR) {
            util::error(db_id, "Unable to create database, defaulting to none");
        }

        util::log("Using new database at location: ", db_path);

        return;
    }

    // If default then use head
    if (id == DB_DEFAULT) {
        db_id = databases.back();
        db_path = "data/pin_db_" + std::to_string(db_id) + "/";

        util::log("Using default database at location: ", db_path);
        return;
    }

    // Otherwise fetch db
    if (std::find(databases.begin(), databases.end(), id) != databases.end()) {
        // It exists
        db_id = id;
        db_path = "data/pin_db_" + std::to_string(id) + "/";

        util::log("Using database at location: ", db_path);
    } else {
        // Doesn't exist
        util::log("Unable to find database, defaulting to none");
        db_id = DB_NONE;
    }
}

/* Build the pin file system structure and/or index all databases found in it */
int DB_FS::build_FS(std::vector<int>& entries) {
    mut.lock();

    // First check for data dir
    struct stat sb;
    if (stat(DATA_DIR, &sb)) {
        if (mkdir(DATA_DIR, 0777)) {
            return E_FAILED_WRITE;
        }
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

    mut.unlock();
    f.close();

    return E_NONE;
}

/* Build a new database and update index, return the id of the new db or DB_NONE for error */
int DB_FS::build_db() {
    // Find largest id
    int new_id = generate_id();

    // Check for error
    if (new_id == DB_NONE) {
        mut.unlock();
        return DB_ERR;
    }

    mut.lock();

    // Create folder
    std::string path = "data/pin_db_" + std::to_string(new_id) + "/";
    int ret = mkdir(path.c_str(), 0777);

    if (ret == -1) {
        mut.unlock();
        return DB_ERR;
    }

    // Update index
    std::ofstream index("data/index", std::ios::app);
    index << new_id << std::endl;
    index.close();

    // Create the convo file index to hold conversation data and write header
    std::ofstream create_index(path + "convo_index", std::ios::binary);
    struct pin_db_header h;
    h.itemsize = sizeof(Convo);
    h.itemno = 0;   // Nothing for now
    h.type = FILE_TYPE_CONVO_INDEX;
    create_index.write((char*) &h, sizeof(h));
    create_index.close();

    // Create user data file
    std::ofstream create_users(path + "users", std::ios::binary);
    h.itemsize = sizeof(User);
    h.type = FILE_TYPE_USER_INDEX;
    create_users.write((char*) &h, sizeof(h));
    create_users.close();

    // Remember to set path for class!
    this->db_path = path;

    mut.unlock();
    return new_id;
}

/* Generate a new unique db id */
int DB_FS::generate_id() {
    mut.lock();

    // Open index
    std::ifstream f("data/index");
    std::string buf;
    int max = 0;

    int n;
    while (std::getline(f, buf)) {
        n = atoi(buf.c_str());

        max = n > max ? n : max;
    }

    f.close();

    mut.unlock();
    return max + 1;
}

/* Update the item count for a file header */
int DB_FS::update_file_header(std::string file, int count) {
    // Open file and read current header
    std::fstream f(file, std::ios::in | std::ios::out | std::ios::binary);
    pin_db_header h;
    f.read((char*) &h, sizeof(h));

    // Seet to start and write updated header
    f.seekg(0, f.beg);
    h.itemno = count;
    f.write((char*) &h, sizeof(h));

    return E_NONE;
}

/* Read the header of a given file and return errors for wrong data, otherwise return item count */
int DB_FS::read_file_header(std::string file, int type, int size) {
    std::fstream f(file, std::ios::in | std::ios::out | std::ios::binary);

    struct pin_db_header h;
    f.read((char*) &h, sizeof(h));
    f.close();

    if (h.type != type || h.version != PIN_VERSION || h.itemsize != size) {
        return DB_ERR;
    }

    return h.itemno;
}

// ****************************** </Database construction> ****************************** //




// MARK: Users
// ****************************** <User functions> ****************************** //

/* Write a new user to the db */
int DB_FS::add_user(User user) {
    if (db_none()) {
        return DB_NONE;
    }

    // First we encrypt both user keys, or quit if it fails
    if ((secure::encrypt(user.dynamic_key, KEYLEN) != E_NONE) || (secure::encrypt(user.master_key, KEYLEN) != E_NONE)) {
        return DB_ERR;
    }

    mut.lock();

    // Open user file
    std::fstream f(db_path + "users", std::ios::in | std::ios::out | std::ios::binary);
    int count = read_file_header(db_path + "users", FILE_TYPE_USER_INDEX, sizeof(User));

    // Check for error
    if (count == DB_ERR) {
        f.close();
        return DB_ERR;
    }

    // Seek to the end of the file and write the user
    f.seekg(0, f.end);
    f.write((char*) &user, sizeof(user));

    int ret = update_file_header(db_path + "users", count + 1);

    if (ret == DB_ERR) {
        return DB_ERR;
    }
    
    f.close();
    mut.unlock();
    return E_NONE;
}

/* Lookup user id by name, if not found and create is true: create new user */
int DB_FS::get_user_id(User& user, bool newuser) {
    if (db_none()) {
        return DB_NONE;
    }

    // Loop through all users in file, either find them or generate new id of max+1
    std::fstream f(db_path + "users", std::ios::in | std::ios::out | std::ios::binary);
    User buf;
    int max = 0;

    int count = read_file_header(db_path + "users", FILE_TYPE_USER_INDEX, sizeof(User));

    if (count == DB_ERR) {
        return E_FAILED_READ;
    }

    // Seek to first entry, then read each one
    f.seekg(sizeof(pin_db_header), f.beg);

    for (int i = 0; i < count; i++) {
        f.read((char*) &buf, sizeof(buf));

        // Check if this is the right user
        if (std::string(user.name) == std::string(buf.name)) {
            if (secure::validate_user(user, buf)) {
                // We found them, so generate dynamic key
                user.uid = buf.uid;
                if (secure::generate_key(user.dynamic_key) != E_NONE) {
                    return E_GENERIC;
                }

                User u_write = user;   // new struct to use for writing (aka excrypted dynamic key and master key)

                if (secure::encrypt(u_write.dynamic_key, KEYLEN) != E_NONE) {
                    return E_GENERIC;
                }

                // Write new dynamic key to db by seeking back and writing new struct
                memcpy(&u_write.master_key, &buf.master_key, sizeof(u_write.master_key));   // Copy the master key
                f.seekg(-(sizeof(u_write)), std::ios::cur);   // Seek negative [user] bytes from current position
                f.write((char*) &u_write, sizeof(u_write));
                
                return E_NONE;
            } else {
                // Right user, bad password

                // If they're new, then name conflict, if not, bad password
                if (newuser) {
                    return E_CONFLICT;
                } else {
                    return E_DENIED;
                }
            }
        }

        // Otherwise increment max
        max = buf.uid > max ? buf.uid : max;
    }

    // Not found, so create or return err
    if (newuser) {
        user.uid = max + 1;

        // Generate keys
        secure::generate_key(user.master_key);
        secure::generate_key(user.dynamic_key);

        // Save to db
        if (add_user(user) != E_NONE) {
            return E_FAILED_WRITE;
        }

        return E_NONE;
    }

    return E_NOT_FOUND;
}

/* Fetch all users in the database */
int DB_FS::get_all_users(std::vector<User>& users) {
    if (db_none()) {
        return E_NOT_FOUND;
    }

    // Loop through all users in file, either find them or generate new id of max+1
    std::fstream f(db_path + "users", std::ios::in | std::ios::out | std::ios::binary);
    User buf;

    int count = read_file_header(db_path + "users", FILE_TYPE_USER_INDEX, sizeof(User));

    if (count == DB_ERR) {
        return E_FAILED_READ;
    }

    // Seek to first entry, then read each one
    f.seekg(sizeof(pin_db_header), f.beg);

    for (int i = 0; i < count; i++) {
        f.read((char*) &buf, sizeof(buf));

        users.push_back(buf);
    }

    return E_NONE;
}

// ****************************** </User functions> ****************************** //





// MARK: Messages
// ****************************** <Message functions> ****************************** //

/* Write a message to the database and return bytes written */
int DB_FS::write_msg(int cid, p_header header, std::string str) {
    if (db_none()) {
        return DB_NONE;
    }

    mut.lock();

    // Open stream
    std::ofstream f(db_path + "convo_" + std::to_string(cid), std::ios::app);

    f << str << std::endl;

    f.close();

    mut.unlock();
    return E_NONE;  // +1 for newline
}

/* Fetch all messages from the given convo and write them into given vector */
int DB_FS::get_all_messages(int cid, std::vector<std::string>& messages) {
    if (db_none()) {
        return DB_NONE;
    }

    mut.lock();

    // Open convo
    std::ifstream f(db_path + "convo_" + std::to_string(cid));
    std::string buf;
    
    while (std::getline(f, buf)) {
        messages.push_back(buf);
    }

    mut.unlock();
    return E_NONE;
}

/* Fetch up to [count] messages from the given convo (most-recent back) and return them */
int DB_FS::get_messages(int cid, std::vector<std::string>& messages, int count) {
    // NOT IMPLEMENTED
    return -1;
}

// ****************************** </Message functions> ****************************** //





// MARK: Convos
// ****************************** <Convo functions> ****************************** //

/* Create a new convo file (from a convo struct) and update the index and cid */
int DB_FS::create_convo(Convo& c) {
    if (db_none()) {
        return DB_NONE;
    }
    mut.lock();

    // Open index
    std::fstream f(db_path + "convo_index", std::ios::app | std::ios::out | std::ios::binary);

    // Read header
    int count = read_file_header(db_path + "convo_index", FILE_TYPE_CONVO_INDEX, sizeof(Convo));

    c.cid = count + 1;

    // Seek to the end of the file and write the convo
    f.seekg(0, f.end);
    f.write((char*) &c, sizeof(c));

    f.close();

    // Update the file header to reflect the new item
    update_file_header(db_path + "convo_index", c.cid);

    // Finally, create the convo file
    std::ofstream create(db_path + "convo_" + std::to_string(c.cid));
    create.close();

    mut.unlock();
    return c.cid;
}

/* Check if a given user is in a convo */
bool DB_FS::check_convo(Convo c, User user) {
    if (c.global) {
        return true;
    }

    // Loop through convo participant list
    for (int i = 0; i < MAX_CONVO_USERS; i++) {
        if (!strcmp(c.users[i], user.name)) {
            return true;
        }
    }

    return false;
}

/* Fetch entries for convo index available to user */
int DB_FS::get_convo_index(std::vector<Convo>& items, User user, bool all) {
    mut.lock();

    if (db_none()) {
        // If none, create an ephemeral convo
        Convo c;
        c.cid = -1;
        strncpy(c.name, "Phantom", 8);
        c.global = true;

        items.push_back(c);

        mut.unlock();
        return E_NONE;
    }

    // Open index
    std::fstream f(db_path + "convo_index", std::ios::in | std::ios::out | std::ios::binary);

    // Read header
    int count = read_file_header(db_path + "convo_index", FILE_TYPE_CONVO_INDEX, sizeof(Convo));
    if (count == DB_ERR) {
        return E_FAILED_READ;
    }

    // Seek to first entry
    f.seekg(sizeof(pin_db_header), f.beg);

    // Now read all items
    Convo c;
    for (int i = 0; i < count; i++) {
        f.read((char*) &c, sizeof(c));

        // Check if this user can view these
        if (all || check_convo(c, user)) {
            items.push_back(c);
        }
    }

    f.close();

    mut.unlock();
    return E_NONE;
}

// ****************************** </Convo functions> ****************************** //