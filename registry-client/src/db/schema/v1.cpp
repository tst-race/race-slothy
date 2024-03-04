#include "db/schema/v1.h"
#include <stdio.h>

int DbSchemaV1::validate(sqlite3* db){
    if(tableExists(db, (char*) "REGISTRY") == 0){
        printf("REGISTRY table does not exist");
        return 0;
    }

    // Should check if registry table has correct columns
   
    return 1;
}

int DbSchemaV1::create(sqlite3* db){
    printf("Creating V1 Schema");
    setDbVersion(db, 1);

    // Create schema as specified in header file
    const char* sql = 
    "CREATE TABLE REGISTRY ("
        "shard_name TEXT PRIMARY KEY NOT NULL, "
        "shard_json TEXT NOT NULL"
    ");";

    executeStatementNoCallback(db, sql);
    return 0;
}

int DbSchemaV1::updateFromPrior(sqlite3* db){
    // Nothing to do, this is V1
    return 0;
}

int DbSchemaV1::getVersion(){
    return 1;
}