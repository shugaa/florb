#ifndef SQLITECACHE_HPP
#define SQLITECACHE_HPP

#include <sqlite3.h>
#include <time.h>
#include <string>
#include <vector>

class sqlitecache
{
    public:
        sqlitecache(const std::string& url);
        ~sqlitecache();

        int get(int z, int x, int y, std::vector<char> &buf);
        int exists(int z, int x, int y);
        void put(int z, int x, int y, time_t expires, const std::vector<char> &buf);
        void sessionid(const std::string& session);

        enum 
        {
            EXPIRED,
            NOTFOUND,
            FOUND
        };
    private:
        static const std::string stmt_checknew;
        static const std::string stmt_createschema;
        static const std::string stmt_sidget;
        static const std::string stmt_sidcreate;
        static const std::string stmt_insert;
        static const std::string stmt_get;
        static const std::string stmt_exists;
        static const std::string stmt_delete;
        static const std::string stmt_index;
        std::string m_url;
        unsigned long int m_sid;

        // Class wide sqlite handle and instance reference count
        static sqlite3 *m_db;
        static unsigned int m_refcount;
};

#endif // SQLITECACHE_HPP

