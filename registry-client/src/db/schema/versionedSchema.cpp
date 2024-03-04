#include "db/schema/versionedSchema.h"
#include <stdio.h>
#include <string>


int VersionedSchema::setDbVersion(sqlite3* db, int version){
    const char* sql1 = "DELETE FROM VERSION;";
    executeStatementNoCallback(db, sql1);
    std::string sql2 = "INSERT INTO VERSION (version) VALUES (" + std::to_string(version) + ");";
    executeStatementNoCallback(db, sql2.c_str());
    return 0;
}