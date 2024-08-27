CLIENT=p_client.cxx client.cxx client.hxx
SERVER=p_server.cxx server.cxx server.hxx
INTERFACE=interface.cxx interface.hxx
DATABASE=database.cxx database.hxx
NET=net.cxx net.hxx
UTIL=util.cxx util.hxx
SECURE=secure.cxx secure.hxx

# File nonsense for test accounts
TEST_ACCOUNTS_FILE=test_accounts.txt

SHARED=$(NET) $(UTIL) $(SECURE)# Files shared between both
GCC=g++

both: client server

client: $(CLIENT) $(INTERFACE) $(SHARED) defn.hxx
	$(GCC) -o client $(CLIENT) $(INTERFACE) $(SHARED) -lpthread -lncurses -g

server: $(SERVER) $(DATABASE) $(SHARED) defn.hxx
	$(GCC) -o server $(SERVER) $(DATABASE) $(SHARED) -lpthread -g

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
