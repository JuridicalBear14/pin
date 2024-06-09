CLIENT=p_client.cpp client.h
SERVER=p_server.cpp server.h
INTERFACE=interface.cpp interface.h
GCC=g++

both: client server

client: $(CLIENT) $(INTERFACE) defn.h
	$(GCC) -o client $(CLIENT) $(INTERFACE) -lpthread -lncurses -g

server: $(SERVER) defn.h
	$(GCC) -o server $(SERVER) -lpthread -g

interface: $(INTERFACE) defn.h
	$(GCC) -o interface $(INTERFACE) -lncurses -g

run: both
	gnome-terminal -- bash -c './server'
	gnome-terminal -- bash -c './client client1'
	gnome-terminal -- bash -c './client client2'

clean:
	rm server client
