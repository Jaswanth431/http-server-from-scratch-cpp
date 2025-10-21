#ifndef LOGGER_H
#define LOGGER_H
#include<fstream>
#include<string>
using namespace std;

enum LogLevel{
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger{
    private:
        std::ofstream logFile;
        LogLevel level;
    public:
        Logger();
        Logger(string filename, LogLevel logLevel);
        void log(const string &message,LogLevel currLevel);
        void logInfo(const string &message);
        void logError(const string &message);
        void logWarn(const string &message);
        void logDebug(const string &message);
        ~Logger();
};

#endif // LOGGER_H