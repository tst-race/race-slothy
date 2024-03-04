#ifndef __RACE_UTIL_H__
#define __RACE_UTIL_H__



#include <ClrMsg.h>  // ClrMsg

#include <string>  // std::string

namespace RaceUtil {

    std::string getEnvironmentVariable(const std::string &key);

    class race_persona_unset : public std::exception {
    public:
        explicit race_persona_unset(const std::string &message);
        const char *what() const noexcept override;

    private:
        std::string message;
    };

    /**
     * @brief Get the active persona from an environment variable.
     *
     * @return std::string The active persona.
     * @throw rtah::race_persona_unset if the environment variable is not set.
     */
    std::string getPersona();

    /**
     * @brief Create a ClrMsg with the given parameters. Set the time to the current time and set the
     * nonce to some default value (currently 10).
     *
     * @param msg The message to place in the ClrMsg.
     * @param from The sender to place in the ClrMsg.
     * @param to The recipient to place in the ClrMsg.
     * @return ClrMsg The created ClrMsg.
     */
    ClrMsg makeClrMsg(const std::string &msg, const std::string &from, const std::string &to);

    std::int64_t getTimeInMicroseconds();

    std::string getMessageSignature(const ClrMsg &msg);
    std::string testIdFromClrMsg(const ClrMsg &msg);
    std::string_view getRandomStringFromPool(size_t length);
}

#endif