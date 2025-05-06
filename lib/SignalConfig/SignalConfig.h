#ifndef SRC_SIGNALCONFIG_H_
#define SRC_SIGNALCONFIG_H_

#include "Channel.h"
#include "DBCMessage.h"
#include "DBCSignal.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SdFat.h>
#include <string>
#include <unordered_map>
#include <vector>

#define SD_CONFIG SdioConfig(FIFO_SDIO)

// Signal configuration structure
struct SignalConfig {
  std::string name;        // Signal name to find in DBC
  std::string displayName; // Optional display name
  bool log;
  bool logRaw;
  bool transmit;
};

class SignalConfigParser {
public:
  static std::unordered_map<uint32_t, std::vector<Channel *>>
  getChannelMap(const char *signalConfigFilename, const char *dbcFilename);

private:
  static std::vector<DBCMessage *> parseDBCFile(const char *dbcFilename);

  static std::vector<SignalConfig> parseSignalConfigFile(const char *signalConfigFilename);

  static std::unordered_map<uint32_t, std::vector<Channel *>>
  buildChannelMap(const std::vector<SignalConfig> &signalConfigs,
                  std::vector<DBCMessage *> &messages);

  static JsonDocument readJSONFile(const char *filename);
};

#endif