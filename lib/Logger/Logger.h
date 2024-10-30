#include <Arduino.h>
#include <SD.h>

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

class Logger {
    public:
        Logger();

        uint8_t initialize();
        void startLogging();
        void println(char* line);
        void println(char* line, uint32_t sec, uint32_t us);
        void getFilename();
        void setFilename(char* name);
    private:
        String line;
        File activeFile;
        char filename[100];
        void openFile();
        void closeFile();
};

#endif
