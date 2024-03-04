#include "db/schema/schema.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

DbSchema::DbSchema(){

}

DbSchema::~DbSchema(){

}


int DbSchema::executeStatementNoCallback(sqlite3* db, const char* sql){
    char *zErrMsg = 0;

    int rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    
    if( rc != SQLITE_OK ){
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    return 0;
}

int DbSchema::executeStatementWithCallback(sqlite3* db, const char* sql, int(*fn)(void*, int, char**,char**), void* data){
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, fn, data, &zErrMsg);
    
    if( rc != SQLITE_OK ){
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    return 0;
}


int DbSchema::tableExists(sqlite3* db, char* tableName){
    std::string tableNameStr(tableName);
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + tableNameStr + "' LIMIT 1;";
    int exists = 0;
    auto handle_result = [](void* data, int argc, char** argv, char** azColName) {         
        int* result = (int*) data;
        *result = 1;
        return 0;
    };
    executeStatementWithCallback(db, sql.c_str(), handle_result, &exists);
    return exists;
}

int DbSchema::baseTableExists(sqlite3* db){
    return tableExists(db, (char*)"VERSION");
}

int DbSchema::removeAllTables(sqlite3* db){
    const char* sql = 
        "PRAGMA writable_schema = 1;"
        "delete from sqlite_master where type in ('table', 'index', 'trigger');"
        "PRAGMA writable_schema = 0;";
    executeStatementNoCallback(db, sql);

    return 0;
}

int DbSchema::getSchemaVersion(sqlite3* db){
    const char* sql = "SELECT * FROM VERSION LIMIT 1";
    int dbVersion = -1;

    auto handle_result = [](void* data, int argc, char** argv, char** azColName) { 
        int* version = (int*) data;
        *version = atoi(argv[0]);
        return 0;
    };

    executeStatementWithCallback(db, sql, handle_result, &dbVersion);
    return dbVersion;
}

int DbSchema::createBaseTable(sqlite3* db){
   const char* sql = "CREATE TABLE VERSION (Version INT PRIMARY KEY NOT NULL);"; 
   executeStatementNoCallback(db, sql);
   return 0;
}