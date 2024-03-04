#include "registry.h"


#include "race/createPidFile.h"
#include "race/output/NodeDaemonPublisher.h"
#include "race/RaceApp.h"
#include "race/RaceInterfaceBuilder.h"
#include "race/RaceInterface.h"
// #include "racetestapp/UserInputResponseParser.h"

#include <limits.h>
#include <chrono>
#include <thread>
#include <RaceLog.h>

const std::string LOG_PREFIX="SlothyRegistry";

RaceRegistry::RaceRegistry(){}

RaceRegistry::~RaceRegistry(){}

int RaceRegistry::start(bool useRace){
    RaceLog::logInfo(LOG_PREFIX, "::start", "");

    if(!isRunning){
        RaceRegistry::database = new RegistryDatabase((char*)"registry.db");
        RaceRegistry::webServer = new HttpServer(444, database); 
        
        isRunning = true;
        int runResult = run(useRace);
        return 0;
    }else{
        printf("Server is already running\n");
    }   
    return 0;
}

int RaceRegistry::run(bool useRACE){

    if(useRACE){
        RaceLog::setLogLevelFile(RaceLog::LL_DEBUG);

        if (rta::createPidFile() == -1) {
            return -1;
        }

        std::string errorMessage;
        bool validConfigs = false;
        try {
            validConfigs = true;
            printf("RaceInterface initializing\n");

            RaceSdk sdk(RaceInterfaceBuilder::getConfig(), "");
            RaceInterface raceInterface = RaceInterfaceBuilder::build(sdk, database);
            printf("RaceInterface has initialized\n");

            NodeDaemonPublisher publisher;

            std::atomic<bool> isAlive = true;
            std::atomic<bool> isRunning = true;
            std::thread statusThread([&isAlive, &isRunning, &raceInterface, &publisher] {
              while (isAlive) {
                if (isRunning) {
                    // Publish status every 3 seconds, with expiration of 9 seconds (3*3)
                    int period = 3;
                    int ttl_factor = 3;
                    nlohmann::json raceStatus = raceInterface.getSdkStatus();
                    // raceStatus["network-manager-status"] = "PLUGIN_READY";
                    raceStatus["validConfigs"] = true;
                    publisher.publishStatus(raceStatus, period * ttl_factor);
                    std::this_thread::sleep_for(std::chrono::seconds(period));
                }
              }
            });
        
            printf("Race Registry is now running\n");
            
            while (isRunning) {
              std::this_thread::sleep_for(std::chrono::milliseconds(500));
            };
            printf("Race Registry shutting down...\n");
            statusThread.join();
            
         } catch (const std::exception &error) {
            errorMessage = "Exception thrown: TYPE: " + std::string(typeid(error).name()) +
                        " WHAT: " + std::string(error.what());
                        stop();
            return -1;
        } catch (...) {
            std::exception_ptr currentException = std::current_exception();
            errorMessage =
                "an unknown error occurred: " +
                std::string(currentException ? currentException.__cxa_exception_type()->name() :
                                            "null");
            stop();
            return -1;
        }

       
    }
    printf("Registry shutting down...\n");
    this->stop();
    return 0;
}

int RaceRegistry::stop(){
    RaceRegistry::webServer->kill();
    return 0;
}
