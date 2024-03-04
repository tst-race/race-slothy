#ifndef __USER_INPUT_RESPONSE_PARSER_H__
#define __USER_INPUT_RESPONSE_PARSER_H__

#include <istream>
#include <string>

class UserInputResponseParser {
private:
    std::string mFilePath;

public:
    /**
     * @brief Parsing exception class.
     */
    class parsing_exception : public std::exception {
    public:
        explicit parsing_exception(const std::string &_message) : message(_message) {}

        const char *what() const noexcept override {
            return message.c_str();
        }

    private:
        std::string message;
    };

    /**
     * @brief User input response as configured in the user input response JSON file
     */
    struct UserResponse {
        bool answered{false};
        std::string response{""};
        int32_t delay_ms{0};
    };

    /**
     * @brief Construct a user input response parser.
     *
     * @param filePath Path to user response JSON file
     */
    explicit UserInputResponseParser(const std::string &filePath);

    virtual ~UserInputResponseParser() {}

    /**
     * @brief Parse the node-specific user input response file for a response
     * to the given prompt for the specified plugin.
     *
     * The response file will be re-parsed with each call to this function.
     *
     * @param pluginId Plugin ID for which to get a response (or "Common" for common values)
     * @param prompt User input prompt/key
     * @return Configured user response, or an unanswered response if no file found or no response
     *      found for the given prompt/key
     */
    virtual UserResponse getResponse(const std::string &pluginId, const std::string &prompt);

protected:
    /**
     * @brief Parse the given input stream for a response to the given prompt for the specified
     * plugin.
     *
     * This is protected in scope so we can call it as part of testing.
     *
     * @param input JSON content to be parsed
     * @param pluginId Plugin ID for which to get a response (or "Common" for common values)
     * @param prompt User input prompt/key
     * @return Configured user response, or an unanswered response if no file found or no response
     *      found for the given prompt/key
     */
    UserResponse getResponse(std::istream &input, const std::string &pluginId,
                             const std::string &prompt);
};

#endif  // __USER_INPUT_RESPONSE_PARSER_H__