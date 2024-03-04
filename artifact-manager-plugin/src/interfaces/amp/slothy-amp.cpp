#include "slothy-amp.hpp"
#include "util/base64.hpp"
#include "util/logger.hpp"
#include "util/profiler.hpp"


SlothyAMP::~SlothyAMP(){

}

SlothyAMP::SlothyAMP(
    IRaceSdkArtifactManager* sdk
) : Slothy("race-registry-00001"), IRacePluginArtifactManager(), raceSdk(sdk){
    
    Logger* logger = Logger::getInstance();
   
    logger->log(LogLevel::DBG, "SlothyAMP", "Readrace-client-00001 Testfile ./testfiley");
}



nlohmann::json SlothyAMP::get_artifact_registry_info(std::string artifact_name){
    Logger* logger = Logger::getInstance();
    logger->log(LogLevel::DBG, "SlothyRace::calling get_artifact_registry_info", "sending amp message");    
    this->raceSdk->sendAmpMessage(this->registry_address, artifact_name);

    // Await the returned message, blocking until we get it
    std::unique_lock<std::mutex> lk(await_mtx);
   
    bool requestFilled = false;
    auto responseString = nlohmann::json();

    while(!requestFilled){
        await_blocker.wait(lk); 
        this->return_queue_lock.lock();
        for (int i = 0; i < this->return_queue.size(); i++) {
            auto &el = this->return_queue[i];
            if(nlohmann::json::accept(el.c_str())){
                auto json_response = nlohmann::json::parse(el.c_str());
                if(!(
                    json_response.contains("name")
                    && json_response.contains("so_sharded")
                    && json_response.contains("so")
                    && json_response.contains("json_sharded")
                    && json_response.contains("json")
                )){
                    logger->log(LogLevel::WARN, "RACE AMP", "Registry response is missing fields, deleting broken entry");                    
                    i--;
                    this->return_queue.erase(this->return_queue.begin() + i);
                    continue;
                }
                logger->log(LogLevel::DBG, "RACE AMP", "Registry response has all fields required");                    
                
                std::string name =  json_response.at("name");

                if(name != artifact_name){
                    continue;
                }

                logger->log(LogLevel::DBG, "RACE AMP", "Received response for '" + artifact_name + "'");                    
                responseString = json_response;
                this->return_queue.erase(this->return_queue.begin() + i);
                requestFilled = true;
            
            }else{
                logger->log(LogLevel::WARN, "RACE AMP", "Was handed invalid JSON by registry");                    
            }
        }
        this->return_queue_lock.unlock();

    }
    
    return responseString; 
}



// Artifact Manager Plugin specific code 

PluginResponse SlothyAMP::init(const PluginConfig &pluginConfig) {
    
    return PLUGIN_OK;
}

PluginResponse SlothyAMP::acquireArtifact(const std::string &destPath,
                                                               const std::string &fileName) {
    this->download_artifact(fileName, destPath);
    return PLUGIN_OK;
}

PluginResponse SlothyAMP::onUserInputReceived(RaceHandle, bool,
                                                                   const std::string &) {
    return PLUGIN_OK;
}

PluginResponse SlothyAMP::onUserAcknowledgementReceived(RaceHandle) {
    return PLUGIN_OK;
}

PluginResponse SlothyAMP::receiveAmpMessage(
    const std::string &msg /* message */) {
    this->return_queue_lock.lock();
    this->return_queue.push_back(msg);
    this->return_queue_lock.unlock();
    this->await_blocker.notify_all();
    return PLUGIN_OK;
}




IRacePluginArtifactManager *createPluginArtifactManager(IRaceSdkArtifactManager *sdk) {
    return new SlothyAMP(sdk);
}

void destroyPluginArtifactManager(IRacePluginArtifactManager *plugin) {
    delete static_cast<SlothyAMP *>(plugin);
}

const RaceVersionInfo raceVersion = RACE_VERSION;
const char *const racePluginId = "PluginArtifactManagerSlothy";
const char *const racePluginDescription = "Slothy Plugin (Peraton)";


