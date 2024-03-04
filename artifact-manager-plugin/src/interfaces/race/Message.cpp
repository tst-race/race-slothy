#include "interfaces/race/Message.h"

#include <algorithm>  // std::count
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>  // std::exception
#include <vector>     // std::vector

#include "interfaces/race/RaceUtil.h"  // rtah::createRandomString, rtah::tokenizeString

const uint64_t Message::sequenceStringLength = 4;

Message::Message(const std::string &message, const std::string &recipient, Time _sendTime,
                      std::string_view _generated, bool isTa1Bypass,
                      const std::string &ta1BypassRoute) :
    messageContent(message),
    generated(_generated),
    personaOfRecipient(recipient),
    sendTime(_sendTime),
    isTa1Bypass(isTa1Bypass),
    ta1BypassRoute(ta1BypassRoute) {}

std::vector<Message> Message::createMessage(const nlohmann::json &inputMessage) {
    try {
        nlohmann::json payload = inputMessage.at("payload");
        std::string sendType = payload.at("send-type");

        if (sendType == "manual") {
            return parseSendMessage(payload);
        } else if (sendType == "auto") {
            return parseAutoMessage(payload);
        } else if (sendType == "plan") {
            return parseTestPlanMessage(payload);
        } else {
            throw std::invalid_argument("unknown message type: " + sendType);
        }
    } catch (nlohmann::json::exception &error) {
        throw std::invalid_argument("invalid message command: " + std::string(error.what()));
    }
}

std::vector<Message> Message::parseSendMessage(const nlohmann::json &payload) {
    try {
        std::string recipient = payload.at("recipient");
        std::string message = payload.at("message");

        std::string testId = payload.at("test-id");
        if (testId.empty()) {
            testId = "default";
        }

        std::string ta1BypassRoute = payload.at("ta1-bypass-route");
        bool isTa1Bypass = not ta1BypassRoute.empty();

        return {Message(testId + " " + message, recipient, Clock::now(), {}, isTa1Bypass,
                        ta1BypassRoute)};
    } catch (std::invalid_argument &error) {
        throw std::invalid_argument("invalid send command for Message: " +
                                    std::string(error.what()));
    } catch (std::out_of_range &error) {
        throw std::invalid_argument("invalid send command for Message: " +
                                    std::string(error.what()));
    } catch (std::bad_alloc &error) {
        throw std::invalid_argument("FATAL: CRITICAL: invalid send command for Message: " +
                                    std::string(error.what()));
    }
}

std::vector<Message> Message::parseAutoMessage(const nlohmann::json &payload) {
    try {
        std::string recipient = payload.at("recipient");
        uint32_t period = payload.at("period");  // milliseconds
        uint32_t count = payload.at("quantity");
        std::uint64_t messageLength = payload.at("size");

        std::string testId = payload.at("test-id");
        if (testId.empty()) {
            testId = "default";
        }

        std::string message = testId + " " + std::string(sequenceStringLength, '0');

        std::string ta1BypassRoute = payload.at("ta1-bypass-route");
        bool isTa1Bypass = not ta1BypassRoute.empty();

        std::string_view generated;
        if (message.size() < messageLength) {
            message += ' ';
            generated = RaceUtil::getRandomStringFromPool(messageLength - message.size());
        }

        auto now = Clock::now();
        std::vector<Message> messages(
            count, Message(message, recipient, now, generated, isTa1Bypass, ta1BypassRoute));
        for (size_t i = 0; i < messages.size(); ++i) {
            messages[i].sendTime = now + Millis(period * i);
            messages[i].messageContent.replace(testId.size() + 1, sequenceStringLength,
                                               sequenceNumberToString(i));
        }
        return messages;
    } catch (std::invalid_argument &error) {
        throw std::invalid_argument("invalid auto command for Message: " +
                                    std::string(error.what()));
    } catch (std::out_of_range &error) {
        throw std::invalid_argument("invalid auto command for Message: " +
                                    std::string(error.what()));
    }
}

std::vector<Message> Message::parseTestPlanMessage(const nlohmann::json &payload) {
    try {
        std::vector<Message> messages;
        nlohmann::json testPlanJson = payload.at("plan");

        int64_t startTimeMillis =
            std::chrono::duration_cast<Millis>(Clock::now().time_since_epoch()).count();
        startTimeMillis = testPlanJson.value("start-time", startTimeMillis);
        Time startTime = Time(Millis(startTimeMillis));

        std::string testId = testPlanJson.value("test-id", "");
        if (testId.empty()) {
            testId = "default";
        }
        testId += " ";

        std::string ta1BypassRoute = testPlanJson.value("ta1-bypass-route", "");
        bool isTa1Bypass = not ta1BypassRoute.empty();

        auto &messageJson = testPlanJson["messages"];
        for (auto personas = messageJson.begin(); personas != messageJson.end(); ++personas) {
            size_t count = 0;
            for (auto &message : *personas) {
                Time sendTime = startTime + Millis(message.value("time", 0));
                uint64_t size = message.value("size", 0ul);
                std::string messageMessage = message.value("message", "");

                std::string messageContent;
                std::string_view generated;
                if (size > 0) {
                    messageContent = testId + sequenceNumberToString(count++);
                    if (messageContent.size() < size) {
                        messageContent += ' ';
                        generated = RaceUtil::getRandomStringFromPool(size - messageContent.size());
                    }

                } else {
                    messageContent = testId + messageMessage;
                }

                messages.emplace_back(messageContent, personas.key(), sendTime, generated,
                                      isTa1Bypass, ta1BypassRoute);
            }
        }

        // sort so that earliest messages to be sent are at the beginning
        std::sort(messages.begin(), messages.end(),
                  [](const Message &a, const Message &b) { return a.sendTime < b.sendTime; });

        return messages;
    } catch (std::exception &error) {
        throw std::invalid_argument("invalid test plan" + std::string(error.what()));
    }
}

std::string Message::sequenceNumberToString(size_t sequenceNumber) {
    std::string sequenceNumberString = std::to_string(sequenceNumber);
    if (sequenceNumberString.size() < sequenceStringLength) {
        sequenceNumberString =
            std::string(sequenceStringLength - sequenceNumberString.size(), '0') +
            sequenceNumberString;
    } else {
        sequenceNumberString =
            std::string(sequenceNumberString.end() - static_cast<uint64_t>(sequenceStringLength),
                        sequenceNumberString.end());
    }

    return sequenceNumberString;
}