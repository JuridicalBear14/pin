CLIENT=p_client.cxx client.cxx
SERVER=p_server.cxx server.cxx
INTERFACE=interface.cxx
DATABASE=database.cxx
SERVER_CONTROL=server_control.cxx
NET=net.cxx
UTIL=util.cxx
SECURE=secure.cxx

SHARED_HEADERS=net.hxx util.hxx secure.hxx defn.hxx
SERVER_HEADERS=server.hxx database.hxx server_control.hxx $(SHARED_HEADERS)
CLIENT_HEADERS=client.hxx interface.hxx $(SHARED_HEADERS)

# File nonsense for test accounts
TEST_ACCOUNTS_FILE=test_accounts.txt

SHARED=$(NET) $(UTIL) $(SECURE)# Files shared between both
GCC=g++

both: client server

client: $(CLIENT) $(INTERFACE) $(SHARED) $(CLIENT_HEADERS)
	$(GCC) -o client $(CLIENT) $(INTERFACE) $(SHARED) -lpthread -lncurses -g -std=c++17

server: $(SERVER) $(DATABASE) $(SERVER_CONTROL) $(SHARED) $(SERVER_HEADERS)
	$(GCC) -o server $(SERVER) $(DATABASE) $(SERVER_CONTROL) $(SHARED) -lpthread -g -std=c++17

# interface: $(INTERFACE) defn.hxx
# 	$(GCC) -o interface $(INTERFACE) -lncurses -g

# database: $(DATABASE) defn.hxx
# 	$(GCC) -o database $(DATABASE) -g

run: both
	gnome-terminal -- bash -c './server'
	gnome-terminal -- bash -c './client local $(shell sed -n '1,1 p' $(TEST_ACCOUNTS_FILE) | tr -d '\n')'
	gnome-terminal -- bash -c './client local $(shell sed -n '2,2 p' $(TEST_ACCOUNTS_FILE) | tr -d '\n')'

# Run just one client
runc: both
	./client local $(shell sed -n '1,1 p' $(TEST_ACCOUNTS_FILE) | tr -d '\n')

clean:
	rm server client
