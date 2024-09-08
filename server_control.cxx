#include "server_control.hxx"

// MARK: Base
// ****************************** <Base commands> ************************* //

Server_control::Server_control(Server* s) {
    server = s;
}

/* Start the scontrol loop */
void Server_control::start_scontrol() {
    // For now just starts the event loop
    user_loop();
}

/* Shutdown the server */
void Server_control::shutdown() {
    // First we disconnect all users
    util::log("Server shutdown: disconnecting all users...");
    server->disconnect_all(true);   // True to permanently close slots

    util::log("Disconnect complete, stopping server...");

    exit(0);
}

/* Disconnect user(s) */
void Server_control::disconnect() {
    server->disconnect_all(false);   // False b/c we don't want to permanently close those slots
}

// ****************************** </Base commands> ************************* //





// MARK: List
// ****************************** <List> ************************* //

void Server_control::list_manager(std::vector<std::string> tokens) {
    if (tokens.size() < 2) {
        util::error(E_BAD_VALUE, "Too few arguments for \"list\" command");
        return;
    }

    int err;

    // Lookup command
    if (tokens[1] == "users" || tokens[1] == "user") {
        // List all or only connected?
        bool all = false;
        if (tokens.size() > 2 && (tokens[2] == "all" || tokens[2] == "-a")) all = true;

        list_users(all);
    } else if (tokens[1] == "convo" || tokens[1] == "convos") {
        list_convos();
    } else {
        util::error(E_BAD_VALUE, "Unknown option for \"list\" command");
        return;
    }
}

void Server_control::list_convos() {

}

void Server_control::list_users(bool all) {
    std::vector<User> users;

    // If (all) then call db list, otherwise only connected users so use server list
    if (all) {
        // Call db
        int ret = server->database->get_all_users(users);

        if (ret != E_NONE) {
            util::error(ret, "Failed to fetch users from database");
            return;
        }

        if (users.size() == 0) {
            std::cout << "| No users saved |\n";
            return;
        }
    } else {
        // Pull users from server list
        for (int i = 0; i < MAXUSR; i++) {
            if (server->pollfds[i].fd != -1) {  // If valid slot
                users.push_back(server->users[i]);
            }
        }

        if (users.size() == 0) {
            std::cout << "| No users connected |\n";
            return;
        }
    }

    // Now print out vector
    char buf[1024];  // String to construct our message into
    for (User u : users) {
        std::snprintf(buf, sizeof(buf), "| Name: %-15s | id: %-2d | cid: %-2d |\n", u.name, u.uid, u.cid);

        std::cout << buf;
    }
}

// ****************************** </List> ************************* //





// MARK: Modify
// ****************************** <Modify> ************************* //



// ****************************** </Modify> ************************* //





// MARK: Create
// ****************************** <Create> ************************* //

/* Manage and call all functions derived from the create command */
void Server_control::create_manager(std::vector<std::string> tokens) {
    // First check args
    if (tokens.size() < 3) {
        util::error(E_BAD_VALUE, "Too few arguments for \"create\" command");
        return;
    }
    
    int err;

    // Check that the name (token 2) is valid
    char c;
    if ((c = util::char_exclusion(tokens[2])) != 0) {
        util::error(E_BAD_VALUE, std::string("Invalid character for name: ") + c);
        return;
    }

    // Lookup table (again)
    if (tokens[1] == "user") {   // Create user
        err = create_user(tokens[2]);

        if (err != E_NONE) {
            util::error(err, "Unable to create user");
            return;
        }

    } else if (tokens[1] == "convo") {
        err = create_convo(tokens);

        if (err != E_NONE) {
            util::error(err, "Unable to create convo");
            return;
        }
    } else {   // Uknown
        util::error(E_BAD_VALUE, "Unknown option for \"create\" command");
        return;
    }
}

/* Create a new user with given username */
int Server_control::create_user(std::string name) {
    User user;
    int err;

    // Copy name over
    memset(user.name, 0, NAMELEN + 1);
    strncpy(user.name, name.c_str(), NAMELEN);

    // Now call database to create said user
    err = server->database->get_user_id(user, true);

    if (err != E_NONE) {
        return err;
    }

    // User has been created
    util::log(user.uid, name, "User has been created");

    // Write keys to console (without logging)
    std::cout << "| Master key: " << user.master_key << " |\n";
    std::cout << "| Dynamic key: " << user.dynamic_key << " |\n";

    return E_NONE;
}

/* Create a new convo with given name */
int Server_control::create_convo(std::vector<std::string> tokens) {
    Convo convo;
    memset(&convo, 0, sizeof(convo));
    int err;

    std::vector<std::string> users(tokens.begin() + 3, tokens.end());  // Extract user compnent to make it easier later

    // Copy name over
    strncpy(convo.name, tokens[2].c_str(), NAMELEN);
    convo.global = true;

    // Copy user names (if provided)
    int i = 0;
    char c;
    for (std::string s : users) {
        // Check for invalid characters
        if ((c = util::char_exclusion(s)) != 0) {
            util::error(E_BAD_VALUE, std::string("Invalid character for name: ") + c);
            continue;
        }

        strncpy(convo.users[i++], s.c_str(), NAMELEN);
        convo.global = false;
    }

    // Now call database to create said convo
    err = server->database->create_convo(convo);

    if (err == DB_NONE) {
        return DB_ERR;
    }

    // User has been created
    util::log(convo.cid, tokens[2], "Convo has been created");

    return E_NONE;
}

// ****************************** </Create> ************************* //





// MARK: Delete
// ****************************** <Delete> ************************* //



// ****************************** </Delete> ************************* //





// MARK: User loop
// ****************************** <User loop> ************************* //

void Server_control::user_loop() {
    // Run until shutdown called
    std::string buf;
    std::vector<std::string> tokens;
    while (std::getline(std::cin, buf)) {
        // Check for blank enter
        if (buf.size() == 0) {
            continue;
        }

        // Log user command
        util::log("Admin command: ", buf);
        util::tolower(buf);

        // Tokenize into vector
        tokens = util::tokenize(buf);

        // Big input lookup table
        if (tokens[0] == "shutdown") {
            shutdown();
        } else if (tokens[0] == "disconnect") {
            disconnect();
        } else if (tokens[0] == "list") {
            list_manager(tokens);
        } else if (tokens[0] == "edit") {

        } else if (tokens[0] == "create") {
            create_manager(tokens);
        } else if (tokens[0] == "delete") {

        } else {
            std::cout << "| Unkown command |\n";
        }
    }
}

// ****************************** </User loop> ************************* //