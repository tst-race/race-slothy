#ifndef RACE_INTERFACE_FACTORY_H
#define RACE_INTERFACE_FACTORY_H

#include "race/RaceInterface.h"
#include <RaceSdk.h>
#include "db/registryDatabase.h"



class RaceInterfaceBuilder {
    public:
        static RaceInterface build(RaceSdk &sdkCore, RegistryDatabase *db);
        static AppConfig getConfig();
};



#endif
