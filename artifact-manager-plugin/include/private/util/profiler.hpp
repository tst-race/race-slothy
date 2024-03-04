#ifndef __PROFILER_H__
#define __PROFILER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <mutex>
#include <vector>
#include <map>
#include <chrono>

class Profiler {
    public:
        Profiler(Profiler &other) = delete;
        void operator=(const Profiler &) = delete;
        static Profiler* getInstance();

        void beginProfilerSection(std::string sectionName);
        void endProfilerSection(std::string sectionName);
        void enable();

        static void printResults(std::map<std::string,float> results);


        std::map<std::string,float> finalizeTimings();
    private:
        Profiler();
        ~Profiler() = default;

        static Profiler* instance;
        static std::mutex mutex_;

        bool enabled = false;


        std::map<std::string,bool> sectionsOpen;
        std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> sectionsStartTimes;
        std::map<std::string,std::vector<float>> timingsLog;
};

#endif