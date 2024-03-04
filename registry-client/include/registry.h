#ifndef RACE_REGISTRY_H
#define RACE_REGISTRY_H



#include "db/registryDatabase.h"
#include "http/httpServer.h"
#include "race/RaceInterface.h"

class RaceRegistry {
public:
    RaceRegistry();
    ~RaceRegistry();
    
    int start(bool useRace);
    int stop();

    std::atomic<bool> isRunning = false;

private:
    RegistryDatabase* database;
    HttpServer* webServer;
    RaceInterface* raceInterface;

    int run(bool useRACE);
};

#endif