#ifndef DB_SCHEMA_H
#define DB_SCHEMA_H

#include <sqlite3.h>

/*
Base Table

VERSION
+--------------+
| Version(int) |
+--------------+
| $(version #) |
+--------------+
*/

class DbSchema {
public:
    DbSchema();
    ~DbSchema();

    static int baseTableExists(sqlite3* db);
    static int tableExists(sqlite3* db, char* tableName);

    static int getSchemaVersion(sqlite3* db);
    static int createBaseTable(sqlite3* db);
    static int removeAllTables(sqlite3* db);
    
protected:

    // These are NOT sanitized, so they should only be used for internal
    // DB modifications that do not rely on user input
    static int executeStatementNoCallback(sqlite3* db, const char* sql);
    static int executeStatementWithCallback(sqlite3* db, const char* sql, int(*fn)(void*, int, char**,char**), void* data);

};


#endif