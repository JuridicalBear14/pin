#pragma once

#include "defn.hxx"
#include "secure.hxx"
#include "util.hxx"

#define DATA_DIR "data"   // The name of our database directory

// Settings for which db to use
#define DB_NONE -2
#define DB_DEFAULT 0   // Default will use the newest db in index, or create new if none are found
#define DB_NEW -1
// #define DB_ERR -3   // Some error in db space    (already defined in defn)

/* Generic database class */
class Database {
    public:
        virtual int add_user(User user) {return -1;};
        virtual int add_convo() {return -1;};
        virtual int write_msg(int cid, p_header header, std::string str) {return -1;};
        virtual int get_all_messages(int cid, std::vector<std::string>& messages) {return -1;};
        virtual int get_messages(int cid, std::vector<std::string>& messages, int count) {return -1;};
        virtual int get_convo_index(std::vector<Convo>& items, User user, bool all) {return -1;};
        virtual int get_user_id(User& user, bool newuser) {return -1;};
        virtual int get_all_users(std::vector<User>& users) {return -1;};
        virtual int create_convo(Convo& c) {return -1;};

        bool db_none() {return db_id == DB_NONE;};

    protected:
        virtual int build_db() {return -1;};
        int db_id;
        std::mutex mut;
};

/* File system implementation of databse */
class DB_FS: public Database {
    public:
        DB_FS(int id);
        int write_msg(int cid, p_header header, std::string str);
        int add_user(User user);
        int get_all_messages(int cid, std::vector<std::string>& messages);
        int get_messages(int cid, std::vector<std::string>& messages, int count);
        int get_convo_index(std::vector<Convo>& items, User user, bool all);
        int get_user_id(User& user, bool newuser);
        int get_all_users(std::vector<User>& users);
        int create_convo(Convo& c);

    private:
        int build_FS(std::vector<int>& entries);
        int build_db();
        int generate_id();
        int update_file_header(std::string file, int count);
        int read_file_header(std::string file, int type, int size);
        bool check_convo(Convo c, User user);

        std::string db_path;
};

/* SQL version of database (to be implemented in the future) */
class DB_SQL: public Database {

};