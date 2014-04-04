#include <sstream>
#include <cstring>
#include <iostream>
#include <fstream>
#include "settings.hpp"
#include "sqlitecache.hpp"
#include "utils.hpp"

const std::string sqlitecache::stmt_checknew = "\
    SELECT name FROM sqlite_master \
    WHERE type='table' \
    AND name='tiles';";
const std::string sqlitecache::stmt_createschema = "\
    CREATE TABLE tiles( \
      sid INTEGER, \
      z INTEGER, \
      x INTEGER, \
      y INTEGER, \
      expires INTEGER, \
      PRIMARY KEY (sid, z, x, y) \
    );\
    CREATE TABLE sessions( \
      sid INTEGER,\
      url TEXT, \
      PRIMARY KEY (sid) \
    );";
const std::string sqlitecache::stmt_sidget = "\
    SELECT sid FROM sessions \
    WHERE url = '%s';";
const std::string sqlitecache::stmt_sidcreate = "\
    INSERT INTO sessions (sid, url) \
    VALUES (NULL, ?1);";
const std::string sqlitecache::stmt_insert = "\
    INSERT INTO tiles (sid, z, x, y, expires) \
    VALUES (?1, ?2, ?3, ?4, ?5);";
const std::string sqlitecache::stmt_get = "\
    SELECT expires FROM tiles \
    WHERE sid = '%d' AND z = '%d' AND x = '%d' AND y = '%d';";
const std::string sqlitecache::stmt_exists = "\
    SELECT expires FROM tiles \
    WHERE sid = '%d' AND z = '%d' AND x = '%d' AND y = '%d';";
const std::string sqlitecache::stmt_delete = "\
    DELETE FROM tiles \
    WHERE sid = '%d' AND z = '%d' AND x = '%d' AND y = '%d';";
const std::string sqlitecache::stmt_index = "\
    CREATE UNIQUE INDEX IF NOT EXISTS prkey \
    ON tiles (sid, z, x, y); \
    CREATE UNIQUE INDEX IF NOT EXISTS prkey \
    ON sessions (sid);";

sqlite3* sqlitecache::m_db = NULL;
unsigned int sqlitecache::m_refcount = 0;

sqlitecache::sqlitecache(const std::string& url) :
    m_sid(-1)
{
    int rc;

    // Create/open the database if necessary
    if (m_db == NULL) 
    {
        rc = sqlite3_open(url.c_str(), &m_db);
        if (rc != SQLITE_OK)
        {
            m_db = NULL;
            throw 0;
        }
    }

    // Check whether the database schema has previously been created
    sqlite3_stmt *stmt = NULL;
    for (;;)
    {
        rc = sqlite3_prepare(m_db, stmt_checknew.c_str(), stmt_checknew.length(), &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            rc = -1;
            break;
        }

        // Schema already created, nothing to do
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) 
        {
            rc = 0;
            break;
        }
         
        // No schema found, create...
        rc = sqlite3_exec(m_db, stmt_createschema.c_str(), NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            rc = -1;
            break;
        }

        // Create index too
        rc = sqlite3_exec(m_db, stmt_index.c_str(), NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            rc = -1;
            break;
        }

        rc = 0;
        break;
    }

    if (stmt) sqlite3_finalize(stmt);

    // An error occured
    if (rc != 0) 
    {
        if (m_refcount == 0)
        {
            sqlite3_close(m_db);
            m_db = NULL;
        }

        throw 0;
    }

    // One more user for this database connection
    m_refcount++;
};

sqlitecache::~sqlitecache()
{
    if (m_refcount > 0)
        m_refcount--;

    if (m_refcount == 0)
    {
        sqlite3_close(m_db);
        m_db = NULL;
    }
};

void sqlitecache::sessionid(const std::string& session) 
{
    int rc;
    char *query = NULL;
    sqlite3_stmt *stmt = NULL;

    for (;;)
    {

        // Try to get an existing session ID for the currently used server
        query = sqlite3_mprintf(stmt_sidget.c_str(), session.c_str());
        if (!query) 
        {
            rc = -1;
            break;
        }

        rc = sqlite3_prepare(m_db, query, strlen(query), &stmt, NULL);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;
        }

        // SID found
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            m_sid = sqlite3_column_int64(stmt, 0);
            rc = 0;
            break;
        } 

        // SID not found, create
        sqlite3_finalize(stmt); stmt = NULL;
        sqlite3_free(query); query = NULL;

        rc = sqlite3_prepare(m_db, stmt_sidcreate.c_str(), stmt_sidcreate.length(), &stmt, NULL);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;;
        }

        rc = sqlite3_bind_text(stmt, 1, session.c_str(), -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) 
        {
            rc = -1;
            break;    
        }

        m_sid = sqlite3_last_insert_rowid(m_db);
        rc = 0;
        break;
    }

    if (query) sqlite3_free(query);
    if (stmt) sqlite3_finalize(stmt);

    if (rc != 0)
    {
        throw 0;
    }
}

void sqlitecache::put(int z, int x, int y, time_t expires, const std::vector<char> &buf)
{
    if (!m_db)
        return;
    if (buf.size() == 0)
        return;
    if ((z < 0) || (x < 0) || (y < 0))
        return;
    if (m_sid < 0)
        return;

    int rc;
    sqlite3_stmt *stmt = NULL;
    char *query = NULL;
   
    for (;;) 
    {
        // Create the storage directory for the tile if not already present
        std::ostringstream oss;
        
        oss << utils::appdir() << "/tiles"; 
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        oss << "/" << m_sid; 
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        oss << "/" << z;
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        oss << "/" << x;
        if (!utils::exists(oss.str()))
            utils::mkdir(oss.str());

        // Store tile in filesystem
        oss << "/" << y;
        std::ofstream of;
        of.open(oss.str().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

        if (!of.is_open())
        {
            rc = -1;
            break;
        }

        of.write(&(buf[0]), buf.size());
        of.close();

        query = sqlite3_mprintf(stmt_delete.c_str(), m_sid, z, x, y);
        if (!query)
        {
            rc = -1;
            break;
        }

        // Drop the tile if it already exists
        rc = sqlite3_prepare(m_db, query, strlen(query), &stmt, NULL);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) 
        {
            rc = -1;
            break;
        }

        sqlite3_free(query); query = NULL;
        sqlite3_finalize(stmt); stmt = NULL;

        rc = sqlite3_prepare(m_db, stmt_insert.c_str(), stmt_insert.length(), &stmt, NULL);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;
        }

        rc = sqlite3_bind_int(stmt, 1, m_sid);
        if (rc != SQLITE_OK) 
        {
            rc =-1;
            break;
        }
        rc = sqlite3_bind_int(stmt, 2, z);
        if (rc != SQLITE_OK) 
        {
            rc =-1;
            break;
        }
        rc = sqlite3_bind_int(stmt, 3, x);
        if (rc != SQLITE_OK) 
        {
            rc =-1;
            break;
        }
        rc = sqlite3_bind_int(stmt, 4, y);
        if (rc != SQLITE_OK) 
        {
            rc =-1;
            break;
        }
        rc = sqlite3_bind_int(stmt, 5, expires);
        if (rc != SQLITE_OK) 
        {
            rc =-1;
            break;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) 
        {
            rc = -1;
            break;
        }

        rc = 0;
        break;
    }

    if (query) sqlite3_free(query);
    if (stmt) sqlite3_finalize(stmt);

    if (rc != 0)
    {
        throw 0;
    }
}

int sqlitecache::exists(int z, int x, int y)
{
    if (!m_db)
        return false;
    if ((z < 0) || (x < 0) || (y < 0))
        return false;
    if (m_sid < 0)
        return false;

    int rc;
    char *query = NULL;
    sqlite3_stmt *stmt = NULL;
    time_t expires = 0;

    for (;;)
    {
        query = sqlite3_mprintf(stmt_exists.c_str(), m_sid, z, x, y);
        if (!query)
        {
            rc = -1;
            break;
        }

        rc = sqlite3_prepare(m_db, query, strlen(query), &stmt, NULL);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;
        }

        // Execute the statement and see if we have the requested tile
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) 
        {
            rc = NOTFOUND;
            break;
        }

        expires = sqlite3_column_int64(stmt, 0);
        rc = FOUND;

        time_t now = time(NULL);
        if (now > expires)
            rc = EXPIRED;

        break;
    }

    if (query) sqlite3_free(query);
    if (stmt) sqlite3_finalize(stmt);

    if (rc < 0)
    {
        throw 0;
    }

    return rc;
}

int sqlitecache::get(int z, int x, int y, std::vector<char> &buf)
{
    if (!m_db)
        return NOTFOUND;
    if ((z < 0) || (x < 0) || (y < 0))
        return NOTFOUND;
    if (m_sid < 0)
        return NOTFOUND;

    int rc;
    char *query = NULL;
    sqlite3_stmt *stmt = NULL;

    for (;;)
    {
        query = sqlite3_mprintf(stmt_get.c_str(), m_sid, z, x, y);
        if (query == NULL)
        {
            rc = -1;
            break;
        }

        rc = sqlite3_prepare(m_db, query, strlen(query), &stmt, NULL);
        if (rc != SQLITE_OK) 
        {
            rc = -1;
            break;
        }

        // Execute the statement and see if we have the requested tile
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            rc = NOTFOUND;
            break;
        }

        // Return the data
        std::ostringstream oss;
        oss << utils::appdir() << "/tiles/" << m_sid << "/" << z << "/" << x << "/" << y; 

        std::ifstream tf;
        tf.open(oss.str().c_str(), std::ios::in | std::ios::binary);

        if (!tf.is_open())
        {
            rc = NOTFOUND;
            break;
        }

        tf.seekg(0, tf.end);
        size_t msize = tf.tellg();
        tf.seekg(0, tf.beg);
        
        if (msize <= 0)
        {
            tf.close();
            rc = NOTFOUND;
            break;
        }

        buf.resize(msize);
        tf.read(&(buf[0]), msize);
        tf.close();
        
        time_t expires = sqlite3_column_int64(stmt, 0);

        // Tile found
        rc = FOUND;

        // Check wheter this tile has expired
        time_t now = time(NULL);
        if (now > expires)
            rc = EXPIRED;
            
        break;
    }

    if (stmt != NULL) sqlite3_finalize(stmt);
    if (query != NULL) sqlite3_free(query);

    if (rc < 0)
    {
        throw 0;
    }

    return rc;
}

