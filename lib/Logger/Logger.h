#include <Arduino.h>
#include <SD.h>

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

class Logger {
    public:
        Logger();

        uint8_t Logger::initialize();
        void Logger::startLogging();
        void Logger::println(char* line);
        void Logger::getFilename();
        void Logger::setFilename(char* name);
    private:
        String line;
        File activeFile;
        char filename[100];
        void Logger::openFile();
        void Logger::closeFile();
};

#endif