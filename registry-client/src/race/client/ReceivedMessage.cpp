#include "race/ReceivedMessage.h"

#include "race/RaceUtil.h"

ReceivedMessage::ReceivedMessage(const ClrMsg &msg) :
    ClrMsg(msg), receivedTime(RaceUtil::getTimeInMicroseconds()) {}
