# pin

Pin is a private, personal messaging application. The server is setup and run by individual owners, and from there the client application serves as an ephemeral portal to connect to the network. There is no data saved in the client, everything is server-side to keep your data on your own machine. The storage of data (or lack thereof) is customizable to the needs of the server owner, and can be partitioned into separate databases for multi-network use on one machine. The client software is a lightweight, terminal based interface which makes it able to run on virtually any computer. It uses a dynamic key system to authenticate users (see below for details) and has no config or save data when run, the only trace is the executable itself. 

Below is an overview of how pin works, how to set it up, and how to use it to the fullest!

---

## Core Concepts
An overview of the core concepts of pin from a user and server owner perspective.

### Users
User accounts are the access points into a pin network, all messages are tied to one of these accounts. Upon startup, the client will prompt the user to create or sign into an account, from there it will verify/register the account with the server according to the server's settings. Users are not required to exist in order to be referenced in other contexts, for example: a convo can be created with participant "user1" even if no account with that name has been registered.

Users have three client-facing attributes, a username and two keys (more info on keys in the next section). Usernames are how you can identify users, and as such all usernames are required to be unique (except when on a network with no database). They are case sensitive and allow most typable characters. As for the server side, users also have a uid (user id) number, each of which are unique positive integers.

### Keys
Keys are the security mechanism for users. Keys are case sensitive and 6 characters by default (but can be customized). They use a dynamic system, which means upon account creation the user will recieve a randomized ```dynamic key``` they can use to log in next. Then, after logging in, the system will assign the user a new ```dynamic key``` and the old key will become invalid. This ensures a compromised key does not compromise the account, which allows users to sign in on other computers with minimal risk.

But what happens if you lose track of your ```dynamic key```? That's where the ```master key``` comes in. This key is assigned once when the account is created and cannot be changed. It has all the same properties as a ```dynamic key```, and can be used to connect as well. The ```master key``` is intended as a backup to protect your account, and shouldn't be used on untrusted machines. Since the ```master key``` cannot be changed, having it compromised means losing your account entirely. The only way to remedy this situation is to have the server owner delete your account and re-register with a new key.

### Convos
```Convos``` (short for "conversations") are where messages are exchanged. Each ```convo``` acts as an independent chat room, with messages being contained to that ```convo```. For example: a network might have one ```convo``` named "sports" for discussing recent games, and another ```convo``` named "food" to discuss food. These separate ```convos``` allow discussion to stay organized on a network with many users. ```Convos``` are global by default, which means all users have access to them. They can be made exclusive by specifying users during creation. A ```convo``` that isn't global will only be visible to users who are included in it, and can only have messages sent from included users. ```Convos``` have a maximum of 10 participants by default (when non-global), but this can be configured by the server owner.

```Convos``` have two other attributes: a cid (convo id) and a name. The cid is a unique positive integer used to distinguish ```convos```, and the name is the user facing identification. ```Convo``` names have all the same restrictions as user names, except that they don't have to be unique. Since there can be many duplicate ```convos``` with different users, the cid is important for server owners to keep track of them.

---

## Server setup and use
To get started, compile the executable with ```make server```. It can be run from the terminal with one optional command line argument of an output file for logging (logs will default to the terminal).
``` BASH
./server

# Log file
./server logs.txt
```
*Note: because certain compile-time settings (such as key length) affect both client and server applications, it's important to make sure these settings are the same across the network. Server owners should compile both executables together with ```make both```, or use a shared settings file to ensure compatability.*

The server will also read from a ```pin.conf``` file in the same directory, this is where you can configure it at runtime (if no file is found it will use default settings automatically). Here's a template for all of the server commands set to their defaults:
```
# <- comment

# Database: -2 none, -1 new, 0 default, 1... specific id
db 0
```

### The Database
The database settings in the config file determine where and how pin will save data (user accounts, convos, messages, etc.). pin uses a multi-database system, which means that user data can be separated into different "databases" depending on the server owner's needs. Each database is self-contained, so user accounts, convos, and message histories are completely independent from one database to another. It's like running a completely separate instance of the server with a different data directory, but managed automatically. This allows a server owner to run multiple different networks on one machine without needing to create separate directories.

- The server will use the ```DB_DEFAULT``` setting by default, this will use the most recent database or create a new one if none are found. For most uses this is the ideal setting, it will save everything as you would expect. 
- To create and use a new database on startup, use the ```DB_NEW``` setting.
- Alternatively, you can also set the server to not save data at all with the ```DB_NONE``` command, this will create one "phantom" global convo for all users. This means no data at all is saved (since the user doesn't cache data), so leaving the convo and returning will permanently delete the previous messages for a user.
- Finally, you can specify a database id (found in the startup section of the server's logs or by looking through the database directories).

*A note about security: as of the current version of pin, only user keys are encrypted in storage. This means usernames, convo names, and messages are stored in plaintext and as such you shouldn't put any sensitive info in them. I plan to upgrade the encryption in the future.*

### Server commands
Once the server is running, there are a number of terminal commands to control the server.
- ```shutdown```
  - Will disconnect all users and end the server process.
- ```disconnect```
  - Will disconnect all users but leave slots open for reconnection.
- ```list```
  - General command for listing information, has two subsections:
  - ```convos```
    - List all convos (with associated information).
  - ```users```
    - List all users currently connected to the network. The ```-a``` or ```all``` argument can be used to list all users stored in the database.
- ```create```
  - General command for creating database items, has two subsections:
  - ```convo```
    - Create a new convo. The first argument will be the name, and subsequent arguments will be the user list (no subsequent arguments will create a global convo).
  - ```user```
    - Create a new user account. Takes one argument: the username.
---

## Client setup and use
The pin client is the access point for users of a network. To create the client executable, compile it with ```make client```.  The client can be run with up to 3 options: IP, name, and key (in that order). By default, the client will startup in local mode (only connecting to the local machine) and prompt the user to log in with a name and key. To specify a remote server, you can provide an IP address (or "local" to stay in local mode) as seen below. The other two options allow users to bypass the login screen by providing credentials ahead of time, as seen below.
```BASH
# Default startup in local mode (will prompt login)
./client

# Startup with connection to external IP (will prompt login)
./client 192.168.0.1

# Startup on local with username provided (will still ask for key)
./client local username

# Startup with remote connection and credentials provided (will launch directly into main menu)
./client 192.168.0.1 username key123
```











