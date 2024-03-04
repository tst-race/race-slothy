#include "interfaces/race/ReceivedMessage.h"

#include "interfaces/race/RaceUtil.h"

ReceivedMessage::ReceivedMessage(const ClrMsg &msg) :
    ClrMsg(msg), receivedTime(RaceUtil::getTimeInMicroseconds()) {}
