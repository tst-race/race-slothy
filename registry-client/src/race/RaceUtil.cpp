#include "race/RaceUtil.h"

#include <chrono>        // std::chrono::system_clock
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdexcept>   
#include <memory>
#include <sstream> 
#include <iomanip>
#include <random> 


std::string createRandomString(size_t length) {
    static const size_t lengthLimit = 10000000;
    if (length > lengthLimit) {
        throw std::invalid_argument("can not create strings larger than " +
                                    std::to_string(lengthLimit) + " bytes");
    }
    std::string message;
    message.reserve(length);
    // TODO: use ascii characters 9, 32-126
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::default_random_engine rand{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, sizeof(alphanum) - 2);
    for (size_t i = 0; i < length; ++i) {
        message.push_back(alphanum[dist(rand)]);
    }
    return message;
}

std::string_view RaceUtil::getRandomStringFromPool(size_t length) {
    static const size_t lengthLimit = 10000000;
    static const std::string pool = createRandomString(lengthLimit);
    thread_local std::default_random_engine rand{std::random_device{}()};
    if (length > lengthLimit) {
        throw std::invalid_argument("can not create strings larger than " +
                                    std::to_string(lengthLimit) + " bytes");
    }

    std::uniform_int_distribution<std::size_t> dist(0, lengthLimit - length);
    size_t begin_index = dist(rand);
    return std::string_view(pool.data() + begin_index, length);
}


std::string RaceUtil::getEnvironmentVariable(const std::string &key) {
    const char *envVarValue = getenv(key.c_str());
    if (envVarValue == nullptr) {
        return "";
    }

    return std::string(envVarValue);
}

RaceUtil::race_persona_unset::race_persona_unset(const std::string &_message) : message(_message) {}

const char *RaceUtil::race_persona_unset::what() const noexcept {
    return message.c_str();
}

std::string RaceUtil::getPersona() {
    const std::string personaEnvVarKey = "RACE_PERSONA";
    const std::string persona = getEnvironmentVariable(personaEnvVarKey);
    if (persona.length() == 0) {
        throw race_persona_unset(std::string(
            "Failed to get persona. Please set the persona in an environment variable named: \"" +
            personaEnvVarKey + "\""));
    }
    return persona;
}


static void handleOpensslError() {
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        printf("OpenSSL error\n");
        // rtah::logError(std::string("Error with OpenSSL call: ") + ERR_error_string(err, NULL));
    }
    throw std::logic_error("Error with OpenSSL call");
}


ClrMsg RaceUtil::makeClrMsg(const std::string &msg, const std::string &from, const std::string &to) {
    constexpr std::int32_t INITIAL_TTL = 10;
    return ClrMsg(msg, from, to, RaceUtil::getTimeInMicroseconds(), INITIAL_TTL);
}


std::int64_t RaceUtil::getTimeInMicroseconds() {
    using namespace std::chrono;
    // C++20 adds utc_clock, which would be better to use here
    auto time = system_clock::now();
    auto micro = duration_cast<microseconds>(time.time_since_epoch());
    return micro.count();
}


std::string RaceUtil::getMessageSignature(const ClrMsg &msg) {
    constexpr int SIGNATURE_SIZE = 20;
    const EVP_MD *md = EVP_sha1();
    std::uint8_t m[SIGNATURE_SIZE];
    int res;

    // Verify hash size to ensure no writes ococur outside of MsgSignature's memory
    if (std::size_t(EVP_MD_size(md)) != SIGNATURE_SIZE) {
        throw std::logic_error(
            "Unexpected size for hash. Expected: " + std::to_string(SIGNATURE_SIZE) +
            ", Received: " + std::to_string(EVP_MD_size(md)));
    }

    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (ctx == nullptr)
        handleOpensslError();

    res = EVP_DigestInit_ex(ctx.get(), md, NULL);
    if (res != 1)
        handleOpensslError();

    const std::string &message = msg.getMsg();
    res = EVP_DigestUpdate(ctx.get(), message.data(), message.size());
    if (res != 1)
        handleOpensslError();

    const std::string &from = msg.getFrom();
    res = EVP_DigestUpdate(ctx.get(), from.data(), from.size());
    if (res != 1)
        handleOpensslError();

    const std::string &to = msg.getTo();
    res = EVP_DigestUpdate(ctx.get(), to.data(), to.size());
    if (res != 1)
        handleOpensslError();

    auto sentTime = msg.getTime();
    res = EVP_DigestUpdate(ctx.get(), &sentTime, sizeof(sentTime));
    if (res != 1)
        handleOpensslError();

    res = EVP_DigestFinal_ex(ctx.get(), m, NULL);
    if (res != 1)
        handleOpensslError();

    // Log hash for debugging
    std::stringstream s;
    s << std::hex << std::setfill('0');
    for (int i = 0; i < SIGNATURE_SIZE; ++i) {
        s << std::setw(2) << static_cast<int>(m[i]);
    }

    return s.str();
}


std::string RaceUtil::testIdFromClrMsg(const ClrMsg &msg) {
    std::string message = msg.getMsg();
    size_t pos = message.find(" ");
    if (pos != std::string::npos) {
        return std::string(message.c_str(), pos);
    }

    return "";
}



