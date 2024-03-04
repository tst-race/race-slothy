#ifndef __SOURCE_CLIENT_RECEIVED_MESSAGE_H__
#define __SOURCE_CLIENT_RECEIVED_MESSAGE_H__

#include "ClrMsg.h"

/**
 * @brief Class to hold a ClrMsg that was received by the application.
 *
 */
class ReceivedMessage : public ClrMsg {
public:
    /**
     * @brief Don't allow the class to be instantiated without a ClrMsg.
     *
     */
    ReceivedMessage() = delete;

    /**
     * @brief Construct a ReceivedMessage from a ClrMsg. Will set the received time to the current
     * time.
     *
     * @param msg
     */
    explicit ReceivedMessage(const ClrMsg &msg);

    /**
     * @brief The time in milliseconds that the message was received.
     *
     */
    const std::int64_t receivedTime;
};

#endif
