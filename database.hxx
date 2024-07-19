#include "defn.hxx"

#define DATA_DIR "data"

// Settings for which db to use
#define DB_NONE -2
#define DB_DEFAULT 0   // Default will use the newest db in index, or create new if none are found
#define DB_NEW -1

class Database {
    public:
        virtual int add_user(std::string name, int id) {return -1;};
        virtual int add_convo() {return -1;};
        virtual int write_msg(int cid, p_header header, std::string str) {return -1;};
        virtual int get_all_messages(int cid, std::vector<std::string>& messages) {return -1;};
        virtual int get_convo_index(std::vector<Convo>& items) {return -1;};
        virtual int get_user_id(std::string name, bool create) {return -1;};
        virtual int create_convo(std::string name, std::vector<int> users) {return -1;};

    protected:
        virtual int build_db() {return -1;};
        std::mutex mut;
};

class DB_FS: public Database {
    public:
        DB_FS(int id);
        int write_msg(int cid, p_header header, std::string str);
        int add_user(std::string name, int id);
        int get_all_messages(int cid, std::vector<std::string>& messages);
        int get_convo_index(std::vector<Convo>& items);
        int get_user_id(std::string name, bool create);
        int create_convo(std::string name, std::vector<int> users);

    private:
        void build_FS(std::vector<int>& entries);
        int build_db();
        int generate_id();
        int update_file_header(int count);
        int read_file_header(int type, int size);

        int db_id;
        std::string db_path;
};


class DB_SQL: public Database {

};