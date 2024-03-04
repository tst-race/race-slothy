#include "race/output/NodeDaemonPublisher.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

NodeDaemonPublisher::NodeDaemonPublisher() : fifoFd(-1) {
    std::string fifoName = "/tmp/racetestapp-output";
    fifoFd = open(fifoName.c_str(), O_RDWR);
    if (fifoFd == -1) {
        throw std::runtime_error("failed to open fifo");
    }

    int dummyFd = open(fifoName.c_str(), O_RDONLY);
    if (dummyFd == -1) {
        throw std::runtime_error("failed to open dummy file descriptor to read fifo");
    }

    // Ignore the SIGPIPE signal in case the app tries to write to a client fifo that does not
    // have a reader. Otherwise, the process would be killed.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        throw std::runtime_error("failed to ignore SIGPIPE");
    }
}

void NodeDaemonPublisher::publishStatus(const nlohmann::json &status, int ttl) {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%TZ");

    (void)status;

    nlohmann::json statusJson = {
        {"status", {{"timestamp", ss.str()}}},
        {"ttl", ttl},
    };

    statusJson["status"]["RaceStatus"] = status;

    std::string statusString = statusJson.dump();
    statusString += "\n";
    // std::cout << "Publishing status: " << statusString << std::endl;
    ssize_t ret = write(fifoFd, statusString.data(), statusString.size());
    if (ret != static_cast<ssize_t>(statusString.size())) {
        throw std::runtime_error("Failed to write status");
    }
}

void NodeDaemonPublisher::publishBootstrapAction(const std::string &message,
                                                 RaceEnums::BootstrapActionType actionType) {
    nlohmann::json bootstrapAction = {
        {"message", message},
        {"actionType", RaceEnums::bootstrapActionTypeToString(actionType)},
    };

    std::string bootstrapActionString = bootstrapAction.dump();
    bootstrapActionString += "\n";
    std::cout << "Publishing bootstrap action: " << bootstrapActionString << std::endl;
    ssize_t ret = write(fifoFd, bootstrapActionString.data(), bootstrapActionString.size());
    if (ret != static_cast<ssize_t>(bootstrapActionString.size())) {
        throw std::runtime_error("Failed to write bootstrap action");
    }
}