#include "slothy-race.hpp"
#include "util/base64.hpp"
#include "util/logger.hpp"
#include "util/profiler.hpp"

SlothyRace::~SlothyRace(){

}

SlothyRace::SlothyRace(
    RaceSdk* sdk, 
    AppConfig raceConfig, 
    std::string registry_address
) : Slothy(registry_address) {
    
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "SlothyRace", "Booting RACE SDK");

    
    std::shared_ptr<opentracing::Tracer> tracer = createTracer(raceConfig.jaegerConfigPath, sdk->getActivePersona());
    logger->log(LogLevel::DBG, "SlothyRace", "Init OpenTracing");

    race_interface = new RaceInterface(*sdk, tracer, raceConfig);
    logger->log(LogLevel::DBG, "SlothyRace", "RACE Interface built");

    if (!sdk->initRaceSystem(race_interface)) {
        logger->log(LogLevel::ERR, "SlothyRace", "RACE SDK has failed to initialize");

        throw std::logic_error("initRaceSystem failed");
    }

    while(!sdk->isConnected()){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    logger->log(LogLevel::DBG, "SlothyRace", "RACE SDK Ready");
}



nlohmann::json SlothyRace::get_artifact_registry_info(std::string artifact_name){
    Logger* logger = Logger::getInstance();

    nlohmann::json inputCommand ={
        {"type", "send-message"},
        {"payload" , {
            {"send-type",   "manual"},
            {"recipient",   this->get_registry_address()},
            {"message",     artifact_name},
            {"test-id",     ""},
            {"ta1-bypass-route", ""}
        }}
    };    
    
    logger->log(LogLevel::INFO, "SlothyRace::download_artifact", "Sending RACE message to registry");    

    const auto messages = Message::createMessage(inputCommand);
    for(auto &m : messages){
        race_interface->sendMessage(m);
    }

    return race_interface->awaitMessageWithHeader(artifact_name);
}

