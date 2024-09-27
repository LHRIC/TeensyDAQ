#include <Logger.h>
#include <SD.h>

const int chipSelect = BUILTIN_SDCARD;

Logger::Logger(){

}

uint8_t Logger::initialize(){
    Serial.println("Initializing SD card...");
    if (!SD.begin(chipSelect)) {
        Serial.println("Card failed, or not present");
        return 1;
    }
    Serial.println("card initialized.");
    return 0;
}

void Logger::startLogging(){
    // filename[0] = '\0';
    // itoa(millis(), filename, 10);
    Serial.println("Filename is:");
    Serial.println(filename);
}

void Logger::openFile(){
    activeFile = SD.open(filename, FILE_WRITE);
};

void Logger::closeFile(){
    activeFile.close();
}

/**
 * @brief Print a line to the active file
 * @param line The line to print
 * @param sec second timestamp
 * @param us microsecond timestamp
 */
void Logger::println(char* line, uint32_t sec, uint32_t us) {
    openFile();
    activeFile.print(String(sec, DEC) + ", " + String(us, DEC) + ": ");
    activeFile.println(line);
    closeFile();
}

void Logger::println(char* line) {
    openFile();
    activeFile.print(String(millis(), DEC) + ": ");
    activeFile.println(line);
    closeFile();
}

void Logger::getFilename(){
    Serial.println(filename);
}

void Logger::setFilename(char* name){
    strcpy(filename, name);
}