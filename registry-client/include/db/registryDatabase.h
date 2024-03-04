#ifndef REGISTRY_DATABASE_H
#define REGISTRY_DATABASE_H

#include <sqlite3.h> 
#include "db/schema/v1.h"
#include <tuple>
#include <vector>
#include <string>
#include <mutex>

class RegistryDatabase {
public:
    RegistryDatabase(char* db_file);
    ~RegistryDatabase();

    int storeShardEntry(const char* shardName, const char* shardJson);
    std::vector<std::tuple<std::string, std::string>> retrieveAllShardEntries();
    int shardExists(const char* shardName);
    std::string getShardData(const char* shardName);


private:
    sqlite3* db;
    std::mutex dbLock = std::mutex();
    VersionedSchema* currentSchema;

    // Check to see if table with correct schema exists. If not
    // create it
    int tryCreateTable();

    int close();
};

#endif