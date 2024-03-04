# Add the source to the shared library

TARGET_SOURCES(raceregistry PRIVATE
    client/RaceApp.cpp
    client/ReceivedMessage.cpp    
    message/Message.cpp
    output/NodeDaemonPublisher.cpp
    RaceInterfaceBuilder.cpp 
    RaceInterface.cpp 
    UserInputResponseCache.cpp 
    UserInputResponseParser.cpp 
    createPidFile.cpp 
    RaceUtil.cpp
)



