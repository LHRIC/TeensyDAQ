#include <Logger.h>
#include <TeensyThreads.h>

ThreadWrap(Serial, serialUsbThreadSafe3)
#define SERIAL_USB ThreadClone(serialUsbThreadSafe3)

Logger::Logger() {
}

uint8_t Logger::initialize() {
  SERIAL_USB.println("Initializing SD card...");

  // Initialize the SD.
  if (!sdCard.begin(SD_CONFIG)) {
    sdCard.initErrorHalt(&SERIAL_USB);
  }

  SERIAL_USB.println("card initialized.");
  return 0;
}

void Logger::startLogging() {
  // Open and preallocate file
  if (openFile() != 0) {
    SERIAL_USB.println("File Opening Failed");
  }

  SERIAL_USB.println("Filename is:");
  SERIAL_USB.println(filename);

}

uint8_t Logger::openFile() {
  if (!activeFile.open(filename, O_RDWR | O_CREAT | O_TRUNC)) {
    SERIAL_USB.println("open failed");
    return 1;
  }
  if (!activeFile.preAllocate((uint64_t) LOG_FILE_SIZE)) {
    SERIAL_USB.println("preAllocate failed\n");
    activeFile.close();
    return 1;
  }

  ringBuffer.begin(&activeFile);
  return 0;
};

void Logger::closeFile() {
  ringBuffer.sync();
  activeFile.truncate();
  activeFile.close();
}

void Logger::stopLogging() {
  closeFile();
}

/**
 * @brief Print a line to the active file
 * @param line The line to print
 * @param sec second timestamp
 * @param us microsecond timestamp
 */
void Logger::println(char *line, uint32_t sec, uint32_t us) {
  uint32_t ringBufferBytesUsed = ringBuffer.bytesUsed();

  if ((ringBufferBytesUsed + activeFile.curPosition()) >
      (LOG_FILE_SIZE - sizeof(line) + sizeof(sec) + sizeof(us))) {
    // FILE IS FULL!!!
    // TODO: DEAL WITH FULL FILE
    return;
  }

  // Flush sector if block device available
  uint8_t ret = flushSector();

  ringBuffer.print(String(sec, DEC) + ", " + String(us, DEC) + ": ");
  ringBuffer.println(line);
}

void Logger::getFilename() { SERIAL_USB.println(filename); }

void Logger::setFilename(char *name) { strcpy(filename, name); }

uint8_t Logger::flushSector() {
  size_t n = ringBuffer.bytesUsed();

  if (n >= SECTOR_SIZE && !activeFile.isBusy()) {
    if (ringBuffer.writeOut(SECTOR_SIZE) != SECTOR_SIZE) {
      // Failed writeout
      return 1;
    }
  } else {
    // ringBuffer bytesUsed <= 512 or file is currently being written to
    return 1;
  }

  return 0;
}
