#include "race/UserInputResponseCache.h"

#include "IRaceSdkApp.h"
// #include "race/raceTestAppHelpers.h"

using json = nlohmann::json;

static std::string fileName = "user-input-response-cache.json";

UserInputResponseCache::UserInputResponseCache(IRaceSdkApp &sdk) : raceSdk(sdk) {}

std::string UserInputResponseCache::getResponse(const std::string &pluginId,
                                                const std::string &prompt) {
    auto key = pluginId + "." + prompt;
    try {
        return cache.at(key).get<std::string>();
    } catch (std::exception &error) {
        // rtah::logDebug("No cache entry for " + key + ": " + std::string(error.what()));
        throw std::out_of_range(key);
    }
}

bool UserInputResponseCache::cacheResponse(const std::string &pluginId, const std::string &prompt,
                                           const std::string &response) {
    auto key = pluginId + "." + prompt;
    cache[key] = response;
    return writeCache();
}

bool UserInputResponseCache::clearCache() {
    cache = json::object();
    return writeCache();
}

bool UserInputResponseCache::readCache() {
    cache = json::object();
    auto raw = raceSdk.readFile(fileName);
    if (not raw.empty()) {
        std::string contents(raw.begin(), raw.end());
        try {
            cache = json::parse(contents);
            return true;
        } catch (std::exception &error) {
            // rtah::logWarning("Unable to parse user response cache: " + std::string(error.what()));
        }
    }
    return false;
}

bool UserInputResponseCache::writeCache() {
    auto contents = cache.dump();
    auto response = raceSdk.writeFile(fileName, {contents.begin(), contents.end()});
    if (response.status != SDK_OK) {
        // rtah::logWarning("Failed to write to user response cache");
    }
    return response.status == SDK_OK;
}
