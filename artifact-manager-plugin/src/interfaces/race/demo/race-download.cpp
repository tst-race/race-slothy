#include "slothy-race.hpp"

#include <OpenTracingHelpers.h>  // createTracer
#include <RaceLog.h>
#include <RaceSdk.h>
#include <limits.h>
#include <unistd.h>  // sleep

#include <thread>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

#include "racetestapp/RaceApp.h"
#include "racetestapp/RaceTestApp.h"
#include "racetestapp/RaceTestAppOutputLog.h"
#include "racetestapp/raceTestAppHelpers.h"
#include "util/logger.hpp"
#include "util/profiler.hpp"


void exit_handler(int s){
    printf("Caught signal %d\n",s);
    exit(1); 
}



void cb(FILE* f, const char* a, bool b, const char* c, void* d){

    return;
}



AppConfig getConfig(){
    AppConfig config;
    config.persona = rtah::getPersona();
    config.etcDirectory = "/etc/race";
    config.auxDataPath = "/aux_data";
    // Config Files
    config.configTarPath = "/tmp/configs.tar.gz";
    config.baseConfigPath = "/data/configs";
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    config.jaegerConfigPath = config.etcDirectory + "/jaeger-config.yml";
    config.userResponsesFilePath = config.etcDirectory + "/user-responses.json";

    const std::string encryptionTypeEnvVarName = "RACE_ENCRYPTION_TYPE";
    const std::string encryptionType = rtah::getEnvironmentVariable(encryptionTypeEnvVarName);
    if (encryptionType == "ENC_AES") {
        config.encryptionType = RaceEnums::ENC_AES;
    } else if (encryptionType == "ENC_NONE") {
        config.encryptionType = RaceEnums::ENC_NONE;
    } else {
        rtah::logWarning("failed to read valid encryption type from environment variable " +
                            encryptionTypeEnvVarName + ". Read value \"" + encryptionType +
                            "\". Using default encryption type: " +
                            RaceEnums::storageEncryptionTypeToString(config.encryptionType));
    }

    return config;
}


std::int32_t main(int argc, char *argv[]){
    Profiler* p = Profiler::getInstance();
    p->enable();

    if(argc != 4){
        printf("Syntax:\n");
        printf("race-download {registry-address} {artifact-name} {path}\n\n");
        printf("{registry-address}:\tA valid, connected RACE registry\n");
        printf("{artifact-name}:\tThe requested artifact\n");
        printf("{filepath}:\tThe location on disk to write the file out\n");
        exit(0);
    }

    Logger* l = Logger::getInstance();
    l->setLogLevel(LogLevel::DBG);

    auto config = getConfig();

    RaceSdk* raceSdk = new RaceSdk(config);


    std::string profilerSectionName = "total";
    p->beginProfilerSection(profilerSectionName);

    SlothyRace* s = new SlothyRace(raceSdk, config, (char*) argv[1]);

    s->download_artifact(argv[2], argv[3]);

    delete(s);
    p->endProfilerSection(profilerSectionName);


    Profiler::printResults(p->finalizeTimings());

    return 0;
}