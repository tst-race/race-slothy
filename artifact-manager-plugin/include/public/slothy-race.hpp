#ifndef __SLOTHY_RACE_H__
#define __SLOTHY_RACE_H__

#include "racetestapp/RaceApp.h"
#include "racetestapp/RaceTestApp.h"
#include "racetestapp/RaceTestAppOutputLog.h"
#include "racetestapp/raceTestAppHelpers.h"
#include <OpenTracingHelpers.h>  // createTracer
#include <RaceLog.h>
#include <RaceSdk.h>
#include <limits.h>
#include <unistd.h>  // sleep

#include <thread>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

#include "slothy.hpp"

#include "interfaces/race/RaceInterface.h"

#include <queue>
#include <string.h>
#include <vector>

class SlothyRace : public Slothy {
    public:
        SlothyRace(RaceSdk* sdk, AppConfig raceConfig, std::string registry_address);
        virtual ~SlothyRace();

    protected:
    private:
        nlohmann::json get_artifact_registry_info(std::string artifact_name) override;
        RaceInterface* race_interface;
};
#endif