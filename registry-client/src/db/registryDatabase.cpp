#include "db/registryDatabase.h"
#include "db/schema/schema.h"
#include "db/schema/v1.h"
#include <stdio.h>
#include <string.h>
#include <stdexcept>

/*
    CURRENT DATABASE IS VERSION 1
    Please ensure all calls are targeting this schema
*/
RegistryDatabase::RegistryDatabase(char* db_file){
    // Set the database version here
    DbSchemaV1* schema = new DbSchemaV1();
    currentSchema = schema;
    printf("Starting Registry Database\n");
    int result = sqlite3_open(db_file, &db);
    tryCreateTable();
}

RegistryDatabase::~RegistryDatabase(){
    dbLock.lock();
    sqlite3_close(db);
    dbLock.unlock();
}


int RegistryDatabase::tryCreateTable(){
    dbLock.lock();
    int tableExists = DbSchema::baseTableExists(db);
    
    if(tableExists == 0 || DbSchema::getSchemaVersion(db) == -1){
        printf("DB has an invalid (or missing) version number, recreating from scratch\n");
        DbSchema::removeAllTables(db);
        DbSchema::createBaseTable(db);
        currentSchema->create(db);
    }

    int dbVersion = DbSchema::getSchemaVersion(db);
    printf("Current DB version: %d\n", dbVersion);

    printf("Validating schema...");
    int valid = currentSchema->validate(db);
    if(valid == 0){
        printf("no\n");
        printf("Rebuilding DB on version %d\n", currentSchema->getVersion());
        currentSchema->create(db);
    }else{
        printf("ok\n");
    }
    dbLock.unlock();
    return 0;
}


//// Exposed DB interfaces
int RegistryDatabase::storeShardEntry(const char* shardName, const char* shardJson){

    std::string sql = "INSERT INTO REGISTRY (shard_name,shard_json) VALUES (?,?);";
    sqlite3_stmt* stmt; 
    
    dbLock.lock();
    sqlite3_prepare_v2(
        db,            
        sql.c_str(),    
        sql.length(),   
        &stmt,          
        nullptr
    );     

    sqlite3_bind_text(
        stmt,            
        1,                
        shardName, 
        strlen(shardName), 
        SQLITE_STATIC
    );    
    sqlite3_bind_text(
        stmt,            
        2,                
        shardJson, 
        strlen(shardJson), 
        SQLITE_STATIC
    );  
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    dbLock.unlock();

    if(result != SQLITE_DONE){
        return 1;
    }             
    return 0;
}

int RegistryDatabase::shardExists(const char* shardName){
    std::string sql = "SELECT * FROM REGISTRY WHERE shard_name=?;";
    sqlite3_stmt* stmt; 
    
    dbLock.lock();
    sqlite3_prepare_v2(
        db,            
        sql.c_str(),    
        sql.length(),   
        &stmt,          
        nullptr
    );     

    sqlite3_bind_text(
        stmt,            
        1,                
        shardName, 
        strlen(shardName), 
        SQLITE_STATIC
    ); 
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    dbLock.unlock();


    if(result == SQLITE_ROW){
        return 1;
    }else if(result != SQLITE_DONE){
        return 0;
    }else{
        return -1;
    }
}

std::string RegistryDatabase::getShardData(const char* shardName){
    std::string sql = "SELECT * FROM REGISTRY WHERE shard_name=? LIMIT 1;";
    sqlite3_stmt* stmt; 
    
    dbLock.lock();
    sqlite3_prepare_v2(
        db,            
        sql.c_str(),    
        sql.length(),   
        &stmt,          
        nullptr
    );     

    sqlite3_bind_text(
        stmt,            
        1,                
        shardName, 
        strlen(shardName), 
        SQLITE_STATIC
    ); 
    int result = sqlite3_step(stmt);

    if(result == SQLITE_ROW){
        // Coerce byte array into a string
        std::string json = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        sqlite3_finalize(stmt);
        dbLock.unlock();
        return json;
    }else{
        sqlite3_finalize(stmt);
        dbLock.unlock();
        throw std::exception();
    }
}




std::vector<std::tuple<std::string, std::string>> RegistryDatabase::retrieveAllShardEntries(){
    dbLock.lock();

    
    
    dbLock.unlock();
    return std::vector<std::tuple<std::string, std::string>>();
}
