#include "race/RaceInterfaceBuilder.h"
#include <opentracing/tracer.h>
#include "race/output/NodeDaemonPublisher.h"
#include <fstream>
#include "race/RaceUtil.h"

RaceInterface RaceInterfaceBuilder::build(RaceSdk &sdkCore, RegistryDatabase *db){
    AppConfig config = RaceInterfaceBuilder::getConfig();
    std::shared_ptr<opentracing::Tracer> tracer = createTracer(config.jaegerConfigPath, sdkCore.getActivePersona());
    RaceInterface raceApp(sdkCore, tracer, config, db);

                
    bool anyEnabled = false;
    for (auto &channelProps : sdkCore.getAllChannelProperties()) {
        if (channelProps.channelStatus == CHANNEL_ENABLED) {
            anyEnabled = true;
        }
    }
    
    if (not anyEnabled) {
        sdkCore.setEnabledChannels(sdkCore.getInitialEnabledChannels());
    }
    
    if (!sdkCore.initRaceSystem(&raceApp)) {
        throw std::logic_error("initRaceSystem failed");
    }


    return raceApp;
}

AppConfig RaceInterfaceBuilder::getConfig(){
    AppConfig config;
    config.persona = RaceUtil::getPersona();
    config.etcDirectory = "/etc/race";
    // Config Files
    config.configTarPath = "/tmp/configs.tar.gz";
    config.baseConfigPath = "/data/configs";
    // Testing specific files (user-responses.json, jaeger-config.json, voa.json)
    config.jaegerConfigPath = config.etcDirectory + "/jaeger-config.yml";
    config.userResponsesFilePath = config.etcDirectory + "/user-responses.json";
    const std::string encryptionTypeEnvVarName = "RACE_ENCRYPTION_TYPE";
    const std::string encryptionType = RaceUtil::getEnvironmentVariable(encryptionTypeEnvVarName);
    if (encryptionType == "ENC_AES") {
        config.encryptionType = RaceEnums::ENC_AES;
    } else if (encryptionType == "ENC_NONE") {
        config.encryptionType = RaceEnums::ENC_NONE;
    } else {
        printf("Used invalid encryption type from environment variable\n");
        // rtah::logWarning("failed to read valid encryption type from environment variable " +
        //                     encryptionTypeEnvVarName + ". Read value \"" + encryptionType +
        //                     "\". Using default encryption type: " +
        //                     RaceEnums::storageEncryptionTypeToString(config.encryptionType));
    }
    return config;
}
