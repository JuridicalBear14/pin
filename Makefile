CLIENT=p_client.cxx client.cxx client.hxx
SERVER=p_server.cxx server.cxx server.hxx
INTERFACE=interface.cxx interface.hxx
DATABASE=database.cxx database.hxx
NET=net.cxx net.hxx
UTIL=util.cxx util.hxx
SECURE=secure.cxx secure.hxx

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
	gnome-terminal -- bash -c './client local client1 key123'
	gnome-terminal -- bash -c './client local client2 key123'

clean:
	rm server client
