#include "MessageConfig.h"

std::unordered_map<uint32_t, std::vector<Channel>>
MessageConfigParser::getChannelMap(const char *signalConfigFilename, const char *dbcFilename) {
  // First parse DBC JSON file
  auto [dbcMessages, signalsByName] = parseDBCFile(dbcFilename);

  // Then get signal configurations
  auto signalConfigs = parseSignalConfigFile(signalConfigFilename);

  // Finally, build the channel map
  return buildChannelMap(signalConfigs, signalsByName, dbcMessages);
}

JsonDocument MessageConfigParser::readJSONFile(const char *filename) {
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

std::pair<std::vector<DBCMessage>, std::unordered_map<std::string, DBCSignal *>>
MessageConfigParser::parseDBCFile(const char *dbcFilename) {
  std::vector<DBCMessage> dbcMessages;
  std::vector<DBCSignal> dbcSignals;
  std::unordered_map<std::string, DBCSignal *> signalsByName;

  // Parse DBC JSON file
  JsonDocument dbcDoc = readJSONFile(dbcFilename);
  if (dbcDoc.isNull()) {
    Serial.println("Failed to parse DBC file");
    return {dbcMessages, signalsByName};
  }

  // Process DBC messages and signals
  JsonArray messages = dbcDoc["messages"];
  for (JsonObject msgJson : messages) {
    uint32_t id = msgJson["frame_id"];
    std::string name = msgJson["message_name"].as<const char *>();
    uint8_t dlc = msgJson["length"];

    // Create DBCMessage and store it
    dbcMessages.emplace_back(id, name, dlc);
    DBCMessage *message = &dbcMessages.back();

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
      bool isMultiplexor = sigJson["is_multiplexer"] | false;

      bool isMultiplexed = false;
      if (sigJson["multiplexer_signal"] != nullptr) {
        isMultiplexed = true;
      }

      uint8_t multiplexValue = 0;
      if (sigJson["multiplexer_ids"] != nullptr) {
        multiplexValue = sigJson["multiplexer_ids"][0];
      }

      // Create DBCSignal and add to message
      dbcSignals.emplace_back(sigName, startBit, length, scale, offset, isSigned, isBigEndian,
                              isMultiplexor, isMultiplexed, multiplexValue);
      DBCSignal *signal = &dbcSignals.back();
      message->addSignal(signal);

      // Store signal by name for quick lookup
      signalsByName[sigName] = signal;
    }
  }

  Serial.println("Successfully parsed DBC file");
  return {dbcMessages, signalsByName};
}

std::vector<SignalConfig>
MessageConfigParser::parseSignalConfigFile(const char *signalConfigFilename) {
  std::vector<SignalConfig> signalConfigs;

  // Parse signal config JSON file
  JsonDocument configDoc = readJSONFile(signalConfigFilename);
  if (configDoc.isNull()) {
    return signalConfigs;
  }

  // Process signal configurations
  JsonArray signalsJson = configDoc["signals"];
  for (JsonObject sigJson : signalsJson) {
    SignalConfig sigConfig;
    sigConfig.name = sigJson["name"].as<const char *>();
    sigConfig.displayName = sigJson["displayName"] | sigConfig.name;
    sigConfig.log = sigJson["log"] | true;
    sigConfig.logRaw = sigJson["logRaw"] | false;
    sigConfig.transmit = sigJson["transmit"] | true;

    signalConfigs.push_back(sigConfig);
  }

  Serial.println("Successfully parsed signal config file");
  return signalConfigs;
}

std::unordered_map<uint32_t, std::vector<Channel>> MessageConfigParser::buildChannelMap(
    const std::vector<SignalConfig> &signalConfigs,
    const std::unordered_map<std::string, DBCSignal *> &signalsByName,
    std::vector<DBCMessage> &messages) {
  std::unordered_map<uint32_t, std::vector<Channel>> channelMap;

  // Create a map from signal to message for quicker lookups
  std::unordered_map<DBCSignal *, const DBCMessage *> signalToMessage;
  for (auto &message : messages) {
    for (DBCSignal* signal : message.getSignals()) {
      signalToMessage[signal] = &message;
    }
  }

  // Process each signal configuration
  for (const auto &sigConfig : signalConfigs) {
    // Find signal in DBC data
    auto sigIt = signalsByName.find(sigConfig.name);
    if (sigIt == signalsByName.end()) {
      Serial.print("Signal not found in DBC: ");
      Serial.println(sigConfig.name.c_str());
      continue;
    }

    DBCSignal *signal = sigIt->second;

    // Find which message this signal belongs to
    auto msgIt = signalToMessage.find(signal);
    if (msgIt == signalToMessage.end()) {
      Serial.print("Could not find message for signal: ");
      Serial.println(sigConfig.name.c_str());
      continue;
    }

    const DBCMessage *message = msgIt->second;
    uint32_t canId = message->getId();

    // Create channel and add to map
    Channel channel(signal, message, sigConfig.displayName, sigConfig.log, sigConfig.logRaw,
                    sigConfig.transmit);
    channelMap[canId].push_back(channel);
  }

  Serial.println("Successfully built channel map");
  return channelMap;
}