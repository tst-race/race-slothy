#include "race/RaceInterface.h"

#include <OpenTracingHelpers.h>  // traceIdFromContext, spanIdFromContext, createTracer

#include <nlohmann/json.hpp>
#include <sstream>  // std::stringstream
#include <thread>   // std::thread

#include "race/Message.h"             // Message
#include <stdexcept>
#include <algorithm>
#include <RaceLog.h>

#include "race/RaceUtil.h"

const std::string LOG_PREFIX="SlothyRegistryClient";

RaceInterface::RaceInterface(
    RaceSdk &_sdk, 
    std::shared_ptr<opentracing::Tracer> _tracer,
    AppConfig &_config,
    RegistryDatabase* _db
) : 
    RaceApp(_sdk, _tracer),
    raceSdk(_sdk),
    config(_config),
    db(_db) {
    RaceLog::logInfo(LOG_PREFIX, " Slothy RaceInterface constructed", "");
}


void RaceInterface::handleReceivedMessage(ClrMsg msg) {
    RaceLog::logInfo(LOG_PREFIX, "::handleReceivedMessage " + msg.getMsg(), "");
    auto ctx = spanContextFromClrMsg(msg);    
    std::shared_ptr<opentracing::Span> span =
        tracer->StartSpan("receiveMessage", {opentracing::FollowsFrom(ctx.get())});

    span->SetTag("source", "raceregistryapp");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageHash", RaceUtil::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());
    span->SetTag("messageTestId", RaceUtil::testIdFromClrMsg(msg));

   
    int8_t ampIndex = msg.getAmpIndex();
    std::string msgBody = msg.getMsg();
    std::string from = msg.getFrom();

    
    handleRegistryMessage(msgBody, from, ampIndex);
}


void RaceInterface::sendResponse(const std::string &msg, const std::string &destination,
                                int8_t ampIndex) {
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("sendMessage");

    ClrMsg clrMsg = ClrMsg(msg, raceSdk.getActivePersona(), destination,
                           RaceUtil::getTimeInMicroseconds(), 0, ampIndex);


    span->SetTag("source", "raceregistry");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(clrMsg.getMsg().size()));
    span->SetTag("messageHash", RaceUtil::getMessageSignature(clrMsg));
    span->SetTag("messageFrom", clrMsg.getFrom());
    span->SetTag("messageTo", clrMsg.getTo());
    span->SetTag("messageTestId", RaceUtil::testIdFromClrMsg(clrMsg));

    raceSdk.sendClientMessage(clrMsg);
}

void RaceInterface::handleRegistryMessage(const std::string &msg, const std::string &persona,
                                         int8_t ampIndex) {

    RaceLog::logInfo(LOG_PREFIX, "::handleRegistryMessage " + msg, "");
    printf("Received registry request\n");
    const char* shard_name = ((std::string) msg.substr(8,-1)).c_str();
    RaceLog::logInfo(LOG_PREFIX, "::handleRegistryMessage: shard name: " + std::string(shard_name), "");
    int shard_exists = db->shardExists(shard_name);
    if(shard_exists == 1){
        try{
            std::string shard_data = db->getShardData(shard_name);
            RaceLog::logInfo(LOG_PREFIX, "::handleRegistryMessage: sending a reponse back to: " + persona, "");
            printf("Sending a response back to %s\n", persona.c_str());
            sendResponse(shard_data, persona, ampIndex);
        }catch(std::exception &e){
          RaceLog::logError(LOG_PREFIX, "::handleRegistryMessage: shard in database but not found", "");
            sendResponse("Shard exists but not found", persona, ampIndex);
        }
    }else{
        RaceLog::logError(LOG_PREFIX, "::handleRegistryMessage: shard not in database", "");
        printf("Sending a notfound response back to %s\n", persona.c_str());
        sendResponse("Shard does not exist in database", persona, ampIndex);
    }
}

AppConfig RaceInterface::getConfig(){
    return config;
}


void RaceInterface::sendMessage(const Message &message) {
    using namespace std::chrono;
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("sendMessage");

    auto start = steady_clock::now();
    std::string messageContent = message.messageContent;
    messageContent.append(message.generated.data(), message.generated.size());
    ClrMsg msg =
        RaceUtil::makeClrMsg(messageContent, raceSdk.getActivePersona(), message.personaOfRecipient);

    auto middle = steady_clock::now();
    printf("Sending the clear message took %lums\n", duration_cast<milliseconds>(middle - start).count());


    span->SetTag("source", "racetestapp");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageHash", RaceUtil::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());
    span->SetTag("messageTestId", RaceUtil::testIdFromClrMsg(msg));

    msg.setTraceId(traceIdFromContext(span->context()));
    msg.setSpanId(spanIdFromContext(span->context()));

    try {
        printf("sending message...\n");
        // printf(((std::string)msg).c_str());
        if (message.isNMBypass) {
            raceSdk.sendNMBypassMessage(msg, message.bypassRoute);
        } else {
            auto handle = raceSdk.sendClientMessage(msg);
            printf("message sent to SDK CORE with handle: %s\n", (std::to_string(handle)).c_str());
            if (not raceSdk.isConnected()) {
                printf(
                    "The client was not ready to send yet (expecting"
                    " onPluginStatusChanged(PLUGIN_READY) call from NetworkManager), so the send may not be "
                    "successful.\n");
            }
        }
    } catch (...) {
#ifndef __ANDROID__
        std::exception_ptr exception = std::current_exception();
        const std::string errorMessage =
            "Exception thrown while sending a message: " +
            std::string(exception ? exception.__cxa_exception_type()->name() :
                                    "exception was null");
#else
        const std::string errorMessage = "Exception thrown while sending a message: ";
#endif
        // rtah::logError(errorMessage);
        // output.writeOutput(errorMessage);
    }
    auto end = steady_clock::now();
    printf("Sending the clear message took %lums\n", duration_cast<milliseconds>(end - middle).count());
}

void RaceInterface::parseAndSendMessage(const nlohmann::json &inputCommand) {
    try {
        const auto messages = Message::createMessage(inputCommand);
        std::thread t1(&RaceInterface::sendPeriodically, this, messages);
        t1.detach();
    } catch (std::invalid_argument &e) {
        // output.writeOutput("ERROR: message: " + inputCommand.dump() + " what: " + e.what());
    }
}

void RaceInterface::sendPeriodically(std::vector<Message> messages) {
    std::stringstream threadId;
    threadId << std::this_thread::get_id();
    // output.writeOutput("sendPeriodically called on thread: " + threadId.str());

    for (auto &message : messages) {
        if (message.messageContent.size() < Message::sequenceStringLength) {
            // output.writeOutput(
            //     "Warning: Message too short for sequence number. Resizing and continuing. "
            //     "message: " +
            //     message.messageContent);
            message.messageContent.resize(Message::sequenceStringLength);
        }

        std::this_thread::sleep_until(message.sendTime);
        sendMessage(message);
    }

    // output.writeOutput("sendPeriodically returned on thread: " + threadId.str());
}

