#include "SignalConfig.h"

std::unordered_map<uint32_t, std::vector<Channel *>>
SignalConfigParser::getChannelMap(const char *signalConfigFilename, const char *dbcFilename) {
  // First parse DBC JSON file
  std::vector<DBCMessage *> dbcMessages = parseDBCFile(dbcFilename);

  // Then get signal configurations
  std::vector<SignalConfig> signalConfigs = parseSignalConfigFile(signalConfigFilename);

  // Finally, build the channel map
  return buildChannelMap(signalConfigs, dbcMessages);
}

JsonDocument SignalConfigParser::readJSONFile(const char *filename) {
  JsonDocument doc;

  SdFs sdCard;
  if (!sdCard.begin(SD_CONFIG)) {
    Serial.println("Failed to initialize SD card");
    return doc;
  }

  FsFile file = sdCard.open(filename, O_RDONLY);
  if (!file) {
    Serial.print("Failed to open file: ");
    Serial.println(filename);
    return doc;
  }

  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse JSON from file: ");
    Serial.println(filename);
  }

  return doc;
}

std::vector<DBCMessage *> SignalConfigParser::parseDBCFile(const char *dbcFilename) {
  std::vector<DBCMessage *> dbcMessages;
  std::vector<DBCSignal> dbcSignals;

  // Parse DBC JSON file
  JsonDocument dbcDoc = readJSONFile(dbcFilename);
  if (dbcDoc.isNull()) {
    Serial.println("Failed to parse DBC file");
    return dbcMessages;
  }

  // Process DBC messages and signals
  JsonArray messages = dbcDoc["messages"];
  if (messages.isNull() || messages.size() == 0) {
    Serial.println("No messages found in DBC file!");
    return dbcMessages;
  }

  for (JsonObject msgJson : messages) {
    uint32_t id = msgJson["frame_id"];
    std::string name = msgJson["message_name"].as<const char *>();
    uint8_t dlc = msgJson["length"];

    // Create DBCMessage and store it
    DBCMessage *message = new DBCMessage(id, name, dlc);
    dbcMessages.push_back(message);

    // Process signals for this message
    JsonArray signalsJson = msgJson["signals"];
    for (JsonObject sigJson : signalsJson) {
      std::string sigName = sigJson["signal_name"].as<const char *>();
      uint8_t startBit = sigJson["start_bit"];
      uint8_t length = sigJson["length"];
      double scale = sigJson["scale"];
      double offset = sigJson["offset"];
      bool isSigned = sigJson["is_signed"];
      bool isBigEndian = sigJson["is_big_endian"];

      // Handle multiplexing
      bool isMultiplexor = false;
      // If is_multiplexer is true and multiplexer_signal is null, then this is a multiplexor
      if (sigJson["is_multiplexer"] && sigJson["multiplexer_signal"] == nullptr) {
        isMultiplexor = true;
      }

      bool isMultiplexed = false;
      if (sigJson["multiplexer_signal"] != nullptr) {
        isMultiplexed = true;
      }

      uint8_t multiplexValue = 0;
      if (sigJson["multiplexer_ids"] != nullptr) {
        multiplexValue = sigJson["multiplexer_ids"][0];
      }

      // Create DBCSignal and add to message
      DBCSignal *signal = new DBCSignal(sigName, startBit, length, scale, offset, isSigned, isBigEndian,
                                   isMultiplexor, isMultiplexed, multiplexValue);
      message->addSignal(signal);
    }
  }

  Serial.println("Successfully parsed DBC file");
  return dbcMessages;
}

std::vector<SignalConfig>
SignalConfigParser::parseSignalConfigFile(const char *signalConfigFilename) {
  std::vector<SignalConfig> signalConfigs;

  // Parse signal config JSON file
  JsonDocument configDoc = readJSONFile(signalConfigFilename);
  if (configDoc.isNull()) {
    return signalConfigs;
  }

  // Process signal configurations
  JsonArray signalsJson = configDoc["signals"];
  for (JsonObject sigJson : signalsJson) {
    SignalConfig sigConfig = {};
    sigConfig.name = sigJson["name"].as<const char *>();
    sigConfig.displayName = sigJson["displayName"].as<const char *>();
    sigConfig.log = sigJson["log"] | true;
    sigConfig.logRaw = sigJson["logRaw"] | false;
    sigConfig.transmit = sigJson["transmit"] | true;

    signalConfigs.emplace_back(sigConfig);
  }

  Serial.println("Successfully parsed signal config file");
  return signalConfigs;
}

std::unordered_map<uint32_t, std::vector<Channel *>>
SignalConfigParser::buildChannelMap(const std::vector<SignalConfig> &signalConfigs,
                                    std::vector<DBCMessage *> &messages) {
  std::unordered_map<uint32_t, std::vector<Channel *>> channelMap;

  // Create a map from signal to message for quicker lookups
  std::unordered_map<std::string, DBCMessage *> signalNameToMessage;
  std::unordered_map<std::string, DBCSignal *> signalsByName;
  for (auto &message : messages) {
    for (auto &signal : message->getSignals()) {
      signalNameToMessage[signal->getName()] = message;
      signalsByName[signal->getName()] = signal;
    }
  }

  // Process each signal configuration
  for (const SignalConfig &sigConfig : signalConfigs) {
    // Find signal in DBC data
    auto sigIt = signalsByName.find(sigConfig.name);
    if (sigIt == signalsByName.end()) {
      Serial.print("Signal not found in DBC: ");
      Serial.println(sigConfig.name.c_str());
      continue;
    }

    DBCSignal *signal = sigIt->second;

    // Find which message this signal belongs to
    auto msgIt = signalNameToMessage.find(signal->getName());
    if (msgIt == signalNameToMessage.end()) {
      Serial.print("Could not find message for signal: ");
      Serial.println(sigConfig.name.c_str());
      continue;
    }

    DBCMessage *message = msgIt->second;
    uint32_t canId = message->getId();

    // Create channel and add to map
    Channel *channel = new Channel(signal, message, sigConfig.displayName, sigConfig.log,
                            sigConfig.logRaw, sigConfig.transmit);
    channelMap[canId].push_back(channel);
  }

  Serial.println("Successfully built channel map");
  return channelMap;
}