// Singleton Logger class
#include "util/logger.hpp"
#include <cstdio>


std::mutex Logger::mutex_;
Logger* Logger::instance = NULL;

Logger::Logger(){
    this->logToFile = false;
    this->logToStdOut = true;
    this->shouldLogProfile = false;
}

Logger* Logger::getInstance(){
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == NULL){
        instance = new Logger();
    }else{
    }

    return instance;
}
std::string Logger::logLevelToString(LogLevel logLevel){
    switch(logLevel){
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERR:
            return "ERR";
        case LogLevel::DBG:
            return "DBG";
        case LogLevel::NONE:
            return "NONE";
    }
    return "";
}
void Logger::setLogLevel(LogLevel ll){
    this->logLevel = ll;
}

bool Logger::enableLogToFile(std::string filePath){
    this->log(LogLevel::INFO, "Logger", "Logging to file has been enabled");
    this->logFile = filePath;
    this->logToFile = true;
    return true;
}

void Logger::enableProfilerLogging(){
    this->shouldLogProfile = true;
}

void Logger::log(LogLevel logLevel, std::string tag, std::string msg){
    std::lock_guard<std::mutex> lock(mutex_);

    if(this->logLevel == LogLevel::NONE){
        return;
    }

    if(logLevel <= this->logLevel){
        // char* output;
        std::string output = "[" 
            + logLevelToString(logLevel) 
            + "]\t["
            + tag
            + "]\t"
            + msg
            + "\n";
        

        if(this->logToStdOut == true){
            printf("%s", output.c_str());
        }

        if(this->logToFile == true){
            FILE* fp = fopen(this->logFile.c_str(), "a");
            fprintf(fp, "%s", output.c_str());
            fclose(fp);
        }
    }
}

