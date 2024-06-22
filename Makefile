CLIENT=p_client.cpp client.cpp client.h
SERVER=p_server.cpp server.cpp server.h
INTERFACE=interface.cpp interface.h
DATABASE=database.cpp database.h
GCC=g++

both: client server

client: $(CLIENT) $(INTERFACE) defn.h
	$(GCC) -o client $(CLIENT) $(INTERFACE) -lpthread -lncurses -g

server: $(SERVER) $(DATABASE) defn.h
	$(GCC) -o server $(SERVER) $(DATABASE) -lpthread -g

interface: $(INTERFACE) defn.h
	$(GCC) -o interface $(INTERFACE) -lncurses -g

database: $(DATABASE) defn.h
	$(GCC) -o database $(DATABASE) -g

run: both
	gnome-terminal -- bash -c './server'
	gnome-terminal -- bash -c './client client1'
	gnome-terminal -- bash -c './client client2'

clean:
	rm server client interface database
