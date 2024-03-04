#ifndef __RACE_INTERFACE_H__
#define __RACE_INTERFACE_H__

#include <opentracing/tracer.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <OpenTracingHelpers.h>  // createTracer


#include "race/RaceApp.h"
#include <RaceConfig.h>
#include "race/Message.h"
#include "race/RaceApp.h"
#include <RaceSdk.h>

#include "db/registryDatabase.h"


class RaceInterface : public RaceApp {
public:
    explicit RaceInterface(
        RaceSdk &_sdk, 
        std::shared_ptr<opentracing::Tracer> tracer, 
        AppConfig &_config,
        RegistryDatabase* _db
    );

   
    void handleRegistryMessage(const std::string &msg, const std::string &persona, int8_t ampIndex);
    void handleReceivedMessage(ClrMsg msg) override;
    void sendMessage(const Message &message);

    AppConfig getConfig();

    RaceSdk &raceSdk;

    void sendPeriodically(std::vector<Message> messages);


    // std::shared_ptr<opentracing::Tracer> tracer;
    void parseAndSendMessage(const nlohmann::json &inputCommand);

    
    // void sendTa1BypassMessage(ClrMsg msg, const std::string &route);
    // virtual void openTa1BypassReceiveConnection(const std::string &persona,
    //                                             const std::string &route) = 0;
    // virtual void rpcDeactivateChannel(const std::string &channelGid) = 0;
    // virtual void rpcDestroyLink(const std::string &linkId) = 0;
    // virtual void rpcCloseConnection(const std::string &connectionId) = 0;
    // virtual void rpcNotifyEpoch(const std::string &data) = 0;



private:
   
    RegistryDatabase* db;
    AppConfig &config;

    bool ready;
    
    nlohmann::json currentSdkStatus;

protected:
    

    void sendResponse(const std::string &msg, const std::string &destination, int8_t ampIndex);


};

#endif
