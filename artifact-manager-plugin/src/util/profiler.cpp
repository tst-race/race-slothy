#include "util/profiler.hpp"
#include "util/logger.hpp"

std::mutex Profiler::mutex_;
Profiler* Profiler::instance = NULL;

Profiler::Profiler(){
    
}

void Profiler::enable(){
    this->enabled = true;
}

void Profiler::printResults(std::map<std::string,float> results){
    Logger* logger = Logger::getInstance();

    // Ensure that if total is logged, print it last.
    std::string total;
    float total_t;
    if(results.count("total")){
        total_t = results["total"];
        total = "total\t" + std::to_string(total_t) + "s";
        results.erase("total");
    }

    logger->log(LogLevel::INFO, "Profiler", "Profiling Results");
    logger->log(LogLevel::INFO, "Profiler", "--------------------------");
    logger->log(LogLevel::INFO, "Profiler", "Section\tTotal Time");
    FILE* fp = fopen("timings.csv", "w+");

    
    

    for (const auto &kvPair : results) {
        auto key = kvPair.first;
        auto value = kvPair.second;
        std::string r = key + "\t" + std::to_string(value) + "s";
        logger->log(LogLevel::INFO, "Profiler", r);
        fprintf(fp, "%s,%f\n", key.c_str(), value);

    }
    if(total != ""){
        logger->log(LogLevel::INFO, "Profiler", total);
        fprintf(fp, "total,%f\n", total_t);
    }

    logger->log(LogLevel::INFO, "Profiler", " Wrote stats to file timings.csv");
    fclose(fp);


}

Profiler* Profiler::getInstance(){
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == NULL){
        instance = new Profiler();
    }else{
    }

    return instance;
}

void Profiler::beginProfilerSection(std::string sectionName){
    if(!this->enabled){
        return;
    }

    Logger* logger = Logger::getInstance();
    if(this->sectionsOpen.count(sectionName) && this->sectionsOpen[sectionName]){
        logger->log(LogLevel::WARN, "Profiler", "Attempting to start profiling on a section already open");
        return;
    }

    logger->log(LogLevel::DBG, "Profiler", "Begin profiling for: " + sectionName);
    this->sectionsOpen[sectionName] = true;
    this->sectionsStartTimes[sectionName] = std::chrono::system_clock::now();
}

void Profiler::endProfilerSection(std::string sectionName){
    if(!this->enabled){
        return;
    }

    Logger* logger = Logger::getInstance();
    if(!this->sectionsOpen.count(sectionName) || !this->sectionsOpen[sectionName]){
        logger->log(LogLevel::WARN, "Profiler", "Attempting to end profiling on a section that never started");
        return;
    }


    this->sectionsOpen[sectionName] = false;

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - this->sectionsStartTimes[sectionName];

    
    if(!this->timingsLog.count(sectionName)){
        this->timingsLog[sectionName] = {};
    }
    this->timingsLog[sectionName].push_back(elapsed_seconds.count());
}

std::map<std::string,float> Profiler::finalizeTimings(){
    std::map<std::string,float> outputMap;

    for (const auto &kvPair : this->timingsLog) {
        auto key = kvPair.first;
        auto value = kvPair.second;

        float sum = 0;
        for (auto& n : value){sum += n;}

        outputMap[key] = sum;
    }

    return outputMap;
}
