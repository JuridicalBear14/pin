/* Implementation for Databases */
#include "database.hxx"

// ********************************** Basic database implementation *********************************************** //



// ********************************** File system database implementation *********************************************** //

/* Constructor for file system database. Id determines which db to use (-1 for none, 0 for make new) */
DB_FS::DB_FS(int id) {
    if (id == DB_NONE) {  // None
        db_id = DB_NONE;
        return;
    } 

    // Build the database file system (or do nothing if it already exists), then return all indexed dbs
    std::vector<int> databases;
    build_FS(databases);
    
    if ((id == DB_NEW) || (id == DB_DEFAULT && databases.size() < 1)) {  // Create new
        db_id = build_db();

        if (db_id == DB_NONE) {
            std::cout << "Unable to create database, defaulting to none\n";
        }

        std::cout << "Using new database at location: " << db_path << "\n";

        return;
    }

    // If default then use head
    if (id == DB_DEFAULT) {
        db_id = databases.back();
        db_path = "data/pin_db_" + std::to_string(db_id) + "/";

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
    mut.lock();

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

    mut.unlock();
    f.close();
}

/* Build a new database and update index, return the id of the new db or DB_NONE for error */
int DB_FS::build_db() {
    // Find largest id
    int new_id = generate_id();

    // Check for error
    if (new_id == DB_NONE) {
        mut.unlock();
        return DB_NONE;
    }

    mut.lock();

    // Create folder
    std::string path = "data/pin_db_" + std::to_string(new_id) + "/";
    int ret = mkdir(path.c_str(), 0777);

    if (ret == -1) {
        mut.unlock();
        return DB_NONE;
    }

    // Update index
    std::ofstream index("data/index", std::ios::app);
    index << new_id << std::endl;
    index.close();

    // Create the convo file index to hold conversation data and write header
    std::ofstream create_index(path + "/convo_index", std::ios::binary);
    struct pin_db_header h;
    h.itemsize = sizeof(Convo);
    h.itemno = 0;   // Nothing for now
    h.type = FILE_TYPE_CONVO_INDEX;
    create_index.write((char*) &h, sizeof(h));
    create_index.close();

    // Create user data file
    std::ofstream create_users(path + "/users", std::ios::app);
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

/* Create a new user for the db */
int DB_FS::add_user(std::string name, int id) {
    mut.lock();

    // Open user file
    std::fstream f(db_path + "/users", std::ios::app);

    std::string buf = std::to_string(id) + ' ' + name;
    f << buf << std::endl;
    
    f.close();
    mut.unlock();
    return id;
}

/* Lookup user id by name, if not found and create is true: create new user and return the new id */
int DB_FS::get_user_id(std::string name, bool create) {
    // Loop through all users in file, either find them or generate new id of max+1
    std::ifstream f(db_path + "/users");
    std::string buf;
    int max = 0;

    int id;
    std::string u_name;
    while (std::getline(f, buf)) {
        // Split into name and id
        id = std::atoi(buf.substr(0, buf.find(' ')).c_str());
        u_name = buf.substr(buf.find(' ') + 1, buf.length());

        // Check if we found the user
        if (name == u_name) {
            return id;
        }

        // Otherwise update max
        max = id > max ? id : max;
    }

    f.close();

    // No user found, either create new or return err
    if (create) {
        id = max + 1;
        return add_user(name, id);
    }


    return -1;
}


/* Write a message to the database and return bytes written */
int DB_FS::write_msg(int cid, p_header header, std::string str) {
    if (db_id == DB_NONE) {
        return DB_NONE;
    }

    mut.lock();

    // Open stream
    std::ofstream f(db_path + "convo_" + std::to_string(cid), std::ios::app);

    f << str << std::endl;

    f.close();

    mut.unlock();
    return str.size() + 1;  // +1 for newline
}

/* Fetch all messages from the given convo and write them into given vector */
int DB_FS::get_all_messages(int cid, std::vector<std::string>& messages) {
    if (db_id == DB_NONE) {
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
    return 0;
}

/* Fetch entries for convo index */
int DB_FS::get_convo_index(std::vector<Convo>& items) {
    if (db_id == DB_NONE) {
        return DB_NONE;
    }

    mut.lock();

    // Open index
    std::fstream f(db_path + "convo_index", std::ios::in | std::ios::out | std::ios::binary);

    // Read header
    int count = read_file_header(FILE_TYPE_CONVO_INDEX, sizeof(Convo));

    // Seek to first entry
    f.seekg(sizeof(pin_db_header), f.beg);

    // Now read all items
    Convo c;
    for (int i = 0; i < count; i++) {
        f.read((char*) &c, sizeof(c));
        items.push_back(c);   
    }

    f.close();

    mut.unlock();
    return 0;
}

/* Create a new convo file (from a convo struct) and update the index and cid */
int DB_FS::create_convo(Convo& c) {
    if (db_id == DB_NONE) {
        return DB_NONE;
    }
    mut.lock();

    // Open index
    std::fstream f(db_path + "convo_index", std::ios::app | std::ios::out | std::ios::binary);

    // Read header
    int count = read_file_header(FILE_TYPE_CONVO_INDEX, sizeof(Convo));

    c.cid = count + 1;

    // Seek to the end of the file and write the convo
    f.seekg(0, f.end);
    f.write((char*) &c, sizeof(c));

    f.close();

    // Update the file header to reflect the new item
    update_file_header(c.cid);

    // Finally, create the convo file
    std::ofstream create(db_path + "convo_" + std::to_string(c.cid));
    create.close();

    mut.unlock();
    return c.cid;
}

/* Update the item count for a file header */
int DB_FS::update_file_header(int count) {
    // Open file and read current header
    std::fstream f(db_path + "convo_index", std::ios::in | std::ios::out | std::ios::binary);
    pin_db_header h;
    f.read((char*) &h, sizeof(h));

    // Seet to start and write updated header
    f.seekg(0, f.beg);
    h.itemno = count;
    f.write((char*) &h, sizeof(h));

    return h.itemno;
}

/* Read the header of a given file and return errors for wrong data, otherwise return item count */
int DB_FS::read_file_header(int type, int size) {
    std::fstream f(db_path + "convo_index", std::ios::in | std::ios::out | std::ios::binary);

    struct pin_db_header h;
    f.read((char*) &h, sizeof(h));
    f.close();

    if (h.type != type || h.version != PIN_VERSION || h.itemsize != size) {
        std::cout << "err\n";
        return -1;
    }

    return h.itemno;
}