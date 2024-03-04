#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <mutex>

enum LogLevel {
    NONE,
    ERR,
    WARN,
    INFO,
    DBG
};

class Logger {
    public:


        Logger(Logger &other) = delete;
        void operator=(const Logger &) = delete;
        static Logger* getInstance();

        bool enableLogToFile(std::string filePath);
        void enableProfilerLogging();

        void log(LogLevel logLevel,std::string tag, std::string msg);
        // void logProfiler(std::string);
        void setLogLevel(LogLevel LogLevel);

    private:
        Logger();
        ~Logger() = default;

        static Logger* instance;
        static std::mutex mutex_;

        LogLevel logLevel = LogLevel::DBG;

        static std::string logLevelToString(LogLevel logLevel);

        bool logToFile;
        bool logToStdOut;
        bool shouldLogProfile;
        std::string logFile;


};


#endif