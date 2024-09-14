# pin

Pin is a private, personal messaging application. The server is setup and run by individual users, and from there the client application serves as an ephemeral portal to connect to the network. There is no data saved in the client, everything is server-side to keep your data on your own machine. The storage of data (or lack thereof) is customizable to the needs of the server owner, and can be partitioned into separate databases for multi-network use on one machine. The client software is a lightweight, terminal based interface which makes it able to run on virtually any computer. It uses a dynamic key system to authenticate users (see below for details) and has no config or save data when run, the only trace is the executable itself. 

Below is an overview of how pin works, how to set it up, and how to use it to the fullest!

---

## Core Concepts
An overview of the core concepts of pin from a user and server owner perspective.

### Users

### Keys

### Convos


---

## Server setup and use
To get started, compile the executable with ```make server```. It can be run from the terminal with one optional command line argument of an output file for logging (logs will default to the terminal).
``` BASH
./server

# Log file
./server logs.txt
```

The server will also read from a ```pin.conf``` file in the same directory, this is where you can configure it (if no file is found it will use default settings automatically). Here's a template for all of the server commands set to their defaults:
```
# <- comment

# Database: -2 none, -1 new, 0 default, 1... specific id
db 0
```

### The Database
The database settings in the config file determine where and how pin will save data (user accounts, convos, messages, etc.). pin uses a multi-database system, which means that user data can be separated into different "databases" depending on the server owner's needs. Each database is self-contained, so user accounts, convos, and message histories are completely independent from one database to another. It's like running a completely separate instance of the server with a different data directory, but managed automatically. This allows a server owner to run multiple different networks on one machine without needing to create separate directories.

- The server will use the ```DB_DEFAULT``` setting by default, this will use the most recent database or create a new one if none are found. For most uses this is the ideal setting, it will save everything as you would expect. 
- To create and use a new database on startup, use the ```DB_NEW``` setting.
- Alternatively, you can also set the server to not save data at all with the ```DB_NONE``` command, this will create one "phantom" global convo for all users. But be careful, this means no data at all is saved (since the user doesn't cache data), so leaving a convo and returning will permanently delete the previous messages for a user.
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
---

## Client setup and use
