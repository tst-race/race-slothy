/*

REGISTRY
+---------------+-------------------+
| shard_id(str) |  shard_url(str)   |
+---------------+-------------------+
| $(Shard ID)   | $(Shard Location) |
+---------------+-------------------+
*/

#ifndef VERSIONED_SCHEMA_H
#define VERSIONED_SCHEMA_H

#include "db/schema/schema.h"
#include <stdio.h>

class VersionedSchema : public DbSchema {
    public:
        virtual int validate(sqlite3* db){return 0;};

        // Assumes that the DB is in an empty state, nothing but the VERSION
        // table with no entries in it
        // Use updateFromPrior() if there is a valid DB that just needs to
        //be migrated
        virtual int create(sqlite3* db){return 0;};
        virtual int updateFromPrior(sqlite3* db){return 0;};
        virtual int getVersion(){return -1;}
    protected:
        int setDbVersion(sqlite3* db, int version);
};
#endif