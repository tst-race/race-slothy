#include "interfaces/race/RaceInterface.h"

#include <OpenTracingHelpers.h>  // traceIdFromContext, spanIdFromContext, createTracer

#include <nlohmann/json.hpp>
#include <sstream>  // std::stringstream
#include <thread>   // std::thread

#include "interfaces/race/Message.h"             // Message
#include <stdexcept>
#include <algorithm>

#include "interfaces/race/RaceUtil.h"
#include "util/logger.hpp"


RaceInterface::RaceInterface(
    RaceSdk &_sdk, 
    std::shared_ptr<opentracing::Tracer> _tracer,
    AppConfig &_config
) : 
    RaceApp(_sdk, _tracer),
    raceSdk(_sdk),
    config(_config)
{
    
}


nlohmann::json RaceInterface::awaitMessageWithHeader(std::string header){
    std::unique_lock<std::mutex> lk(await_mtx);
    Logger* logger = Logger::getInstance();

   
    bool requestFilled = false;
    auto responseString = nlohmann::json();

    while(!requestFilled){
        await_blocker.wait(lk); 
        this->return_queue_lock.lock();
        for (int i = 0; i < this->return_queue.size(); i++) {
            auto &el = this->return_queue[i];
            if(json::accept(el.c_str())){
                auto json_response = json::parse(el.c_str());
                if(!(
                    json_response.contains("name")
                    && json_response.contains("so_sharded")
                    && json_response.contains("so")
                    && json_response.contains("json_sharded")
                    && json_response.contains("json")
                )){
                    logger->log(LogLevel::WARN, "RACEInterface", "Registry response is missing fields, deleting broken entry");                    
                    i--;
                    this->return_queue.erase(this->return_queue.begin() + i);
                    continue;
                }
                logger->log(LogLevel::DBG, "RACEInterface", "Registry response has all fields required");                    
                
                std::string name =  json_response.at("name");

                if(name != header){
                    continue;
                }

                logger->log(LogLevel::DBG, "RACEInterface", "Received response for '" + header + "'");                    
                responseString = json_response;
                this->return_queue.erase(this->return_queue.begin() + i);
                requestFilled = true;
            
            }else{
                logger->log(LogLevel::WARN, "RACEInterface", "Was handed invalid JSON by registry");                    
            }
        }
        this->return_queue_lock.unlock();

    }
    
    return responseString; 
}

void RaceInterface::handleReceivedMessage(ClrMsg msg) {
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

    this->return_queue_lock.lock();
    this->return_queue.push_back(msgBody);
    this->return_queue_lock.unlock();
    this->await_blocker.notify_all();
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
    // printf("Building the clear message took %lums\n", duration_cast<milliseconds>(middle - start).count());


    span->SetTag("source", "slothy-race-downloader");
    span->SetTag("file", __FILE__);
    span->SetTag("messageSize", std::to_string(msg.getMsg().size()));
    span->SetTag("messageHash", RaceUtil::getMessageSignature(msg));
    span->SetTag("messageFrom", msg.getFrom());
    span->SetTag("messageTo", msg.getTo());
    span->SetTag("messageTestId", RaceUtil::testIdFromClrMsg(msg));

    msg.setTraceId(traceIdFromContext(span->context()));
    msg.setSpanId(spanIdFromContext(span->context()));

    try {
        // printf(((std::string)msg).c_str());
        if (message.isTa1Bypass) {
            raceSdk.sendTa1BypassMessage(msg, message.ta1BypassRoute);
        } else {
            auto handle = raceSdk.sendClientMessage(msg);
            if (not raceSdk.isConnected()) {
                printf(
                    "The client was not ready to send yet (expecting"
                    " onPluginStatusChanged(PLUGIN_READY) call from TA1), so the send may not be "
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
    // printf("Sending the clear message took %lums\n", duration_cast<milliseconds>(end - middle).count());
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

