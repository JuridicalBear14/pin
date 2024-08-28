#include "server_control.hxx"

// MARK: Startup
// ****************************** <Startup> ************************* //

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

}

// ****************************** </Startup> ************************* //





// MARK: Accessors
// ****************************** <Accessors> ************************* //



// ****************************** </Accessors> ************************* //





// MARK: Mutators
// ****************************** <Mutators> ************************* //



// ****************************** </Mutators> ************************* //





// MARK: User loop
// ****************************** <User loop> ************************* //

void Server_control::user_loop() {
    // Run until shutdown called
    std::string buf;
    while (true) {
        std::cin >> buf;
        std::cout << buf << "\n";
    }
}

// ****************************** </User loop> ************************* //