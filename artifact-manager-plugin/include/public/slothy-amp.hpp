#ifndef __SLOTHY_AMP_H__
#define __SLOTHY_AMP_H__

#include "slothy.hpp"
#include <nlohmann/json.hpp>
#include <IRaceSdkArtifactManager.h>
#include <IRacePluginArtifactManager.h>
#include <mutex>
#include <queue>
#include <cstdint>
#include <condition_variable>
const char* BUILD_VERSION = "1.1";


class SlothyAMP : public Slothy, public IRacePluginArtifactManager {
  public:
    SlothyAMP(IRaceSdkArtifactManager* sdk);
    ~SlothyAMP();

    // PluginArtifactManager specific calls
    virtual PluginResponse init(const PluginConfig &pluginConfig) override;
    virtual PluginResponse acquireArtifact(const std::string &destPath,
                                           const std::string &fileName) override;
    virtual PluginResponse onUserInputReceived(RaceHandle handle, bool answered,
                                               const std::string &response) override;
    virtual PluginResponse onUserAcknowledgementReceived(RaceHandle handle) override;
    virtual PluginResponse receiveAmpMessage(const std::string &message) override;


  protected:

  private:
    nlohmann::json get_artifact_registry_info(std::string artifact_name) override;

    IRaceSdkArtifactManager *raceSdk;

    std::condition_variable await_blocker;
    std::mutex await_mtx;

    std::mutex return_queue_lock;
    std::vector<std::string> return_queue;
};

#endif