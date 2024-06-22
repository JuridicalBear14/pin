#include "defn.h"
#define DATA_DIR "data"

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
        int build_db();
};


class DB_SQL: public Database {

};