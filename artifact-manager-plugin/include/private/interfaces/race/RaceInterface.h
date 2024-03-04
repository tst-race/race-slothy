#ifndef __RACE_INTERFACE_H__
#define __RACE_INTERFACE_H__

#include <opentracing/tracer.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <OpenTracingHelpers.h>  // createTracer


#include "./RaceApp.h"
#include <RaceConfig.h>
#include "./Message.h"
#include "./RaceApp.h"
#include <RaceSdk.h>
#include <mutex>
#include <queue>


class RaceInterface : public RaceApp {
public:
    explicit RaceInterface(
        RaceSdk &_sdk, 
        std::shared_ptr<opentracing::Tracer> tracer, 
        AppConfig &_config
    );

   
    void handleReceivedMessage(ClrMsg msg) override;
    void sendMessage(const Message &message);

    AppConfig getConfig();

    RaceSdk &raceSdk;

    void sendPeriodically(std::vector<Message> messages);


    void parseAndSendMessage(const nlohmann::json &inputCommand);
    nlohmann::json awaitMessageWithHeader(std::string header);

private:
   
    AppConfig &config;

    bool ready;
    
    nlohmann::json currentSdkStatus;

    std::condition_variable await_blocker;
    std::mutex await_mtx;

    std::mutex return_queue_lock;
    std::vector<std::string> return_queue;

protected:
    

    void sendResponse(const std::string &msg, const std::string &destination, int8_t ampIndex);


};

#endif
