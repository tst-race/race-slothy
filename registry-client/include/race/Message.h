#ifndef __SOURCE_MESSAGE_MESSAGE_H__
#define __SOURCE_MESSAGE_MESSAGE_H__

#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>


/**
 * @brief Class to represent a message in the RaceTestApp.
 *
 */
class Message {
public:
    using Clock = std::chrono::system_clock;
    using Millis = std::chrono::milliseconds;
    using Time = std::chrono::time_point<Clock>;

    Message() = delete;
    Message(const std::string &message, const std::string &recipient, Time sendTime,
            std::string_view _generated, bool isNMBypass, const std::string &bypassRoute);

    /**
     * @brief Create a Message given an application input string.
     *
     * @param inputMessage The input message to the application.
     * @return vector<Message> The resulting Messages to be sent.
     * @throw On error a std::invalid_argument exception is thrown.
     */
    static std::vector<Message> createMessage(const nlohmann::json &inputMessage);

    /**
     * @brief The content of the message stored in a string.
     *
     */
    std::string messageContent;

    /**
     * @brief The randomly generated part of the message, or empty string for manual messages.
     *
     */
    std::string_view generated;

    /**
     * @brief The peronsa of the recipient stored in a string.
     *
     */
    std::string personaOfRecipient;

    Time sendTime;

    /**
     * @brief The message is to be send bypassing NetworkManager processing.
     */
    bool isNMBypass;

    /**
     * @brief Route (connection ID, link ID, or channel ID) by which to send the nm-bypass message.
     */
    std::string bypassRoute;

    /**
     * @brief The number of bytes used to store the sequence number string
     *
     */
    static const uint64_t sequenceStringLength;

private:
    static std::vector<Message> parseSendMessage(const nlohmann::json &payload);
    static std::vector<Message> parseAutoMessage(const nlohmann::json &payload);
    static std::vector<Message> parseTestPlanMessage(const nlohmann::json &payload);

    /**
     * @brief zero pad a number until it's sequenceStringLength
     *
     * @param sequenceNumber The sequence number to insert pad with zeros
     */
    static std::string sequenceNumberToString(size_t sequenceNumber);
};



#endif
