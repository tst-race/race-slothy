/*

REGISTRY
+-----------------+--------------------+
| shard_name(TEXT)|  shard_json(TEXT)  |
+-----------------+--------------------+
| $(Shard Name)   |   $(Shard JSON)    |
+-----------------+--------------------+
*/

#ifndef DB_SCHEMA_V1_H
#define DB_SCHEMA_V1_H

#include "db/schema/versionedSchema.h"

class DbSchemaV1 : public VersionedSchema {
    public:
        int validate(sqlite3* db) override;
        int create(sqlite3* db) override;
        int updateFromPrior(sqlite3* db) override;
        int getVersion() override;
};
#endif