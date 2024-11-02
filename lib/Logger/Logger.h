#include <Arduino.h>
#include <SdFat.h>
#include <RingBuf.h>

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#define SD_CONFIG SdioConfig(FIFO_SDIO)
#define RING_BUF_CAPACITY 100000

// Estimated: 20 byte lines at 100Khz for 10 minutes
#define LOG_FILE_SIZE 20 * 100000 * 600

#define SECTOR_SIZE 512

class Logger {
    public:
        Logger();

        uint8_t initialize();
        void startLogging();
        void println(char* line, uint32_t sec, uint32_t us);
        void getFilename();
        void setFilename(char* name);
        uint8_t flushSector();
    private:
        String line;
        FsFile activeFile;
        SdFs sdCard;
        RingBuf<FsFile, RING_BUF_CAPACITY> ringBuffer;
        char filename[100];
        uint8_t openFile();
        void closeFile();
};

#endif
