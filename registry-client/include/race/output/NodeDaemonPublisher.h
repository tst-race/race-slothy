#ifndef _NODE_DAEMON_PUBLISHER_H_
#define _NODE_DAEMON_PUBLISHER_H_

#include <string>

#include <RaceEnums.h>
#include "nlohmann/json.hpp"

/**
 * @brief Class for sending app status to the node daemon
 *
 */
class NodeDaemonPublisher {
public:
    /**
     * @brief Construct a new Race Test App Input Fifo object.
     *
     * @throw std::runtime_exception if construction fails.
     */
    NodeDaemonPublisher();

    /**
     * @brief publish app status to node daemon
     *
     * @throw std::runtime_exception if writing to fifo fails.
     */
    void publishStatus(const nlohmann::json &status, int ttl);

    void publishBootstrapAction(const std::string &message,
                                RaceEnums::BootstrapActionType actionType);

private:
    int fifoFd;
};

#endif
