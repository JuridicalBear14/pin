#include "defn.h"

#define DATA_DIR "data"

// Settings for which db to use
#define DB_NONE -1
#define DB_DEFAULT 0   // Default will use the newest db in index, or create new if none are found
#define DB_NEW -2

class Database {
    public:
        virtual int add_user() {return -1;};
        virtual int add_convo() {return -1;};
        virtual int add_msg(p_header header, std::string str) {return -1;};

    protected:
        virtual int build_db() {return -1;};
};

class DB_FS: public Database {
    public:
        DB_FS(int id);
        int add_msg(p_header header, std::string str);
        int add_user();

    private:
        void build_FS(std::vector<int>& entries);
        int build_db();

        int db_id;
        std::string db_path;
};


class DB_SQL: public Database {

};