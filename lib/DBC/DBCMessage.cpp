#include "DBCMessage.h"

/**
 * Constructor for the DBCMessage class.
 * @param name The name of the message
 * @param id The message ID
 * @param dlc The message length in bytes
 */
DBCMessage::DBCMessage(uint32_t id, const std::string &name, uint8_t dlc)
    : id(id), name(name), dlc(dlc) {}

/**
 * Adds a signal to the message.
 * @param signal The signal to add
 */
void DBCMessage::addSignal(DBCSignal *signal) { signals.push_back(signal); }

/**
 * Get signals in the message.
 * @return A vector of references to signals in the message
 */
std::vector<DBCSignal *> DBCMessage::getSignals() {
  return std::vector<DBCSignal *>(signals.begin(), signals.end());
}

/**
 * Process a message and extract the signal values.
 * @param data The raw message data
 */
void DBCMessage::processMessage(const uint8_t *data) {
  // Get multiplexor if exists
  uint8_t multiplexValue = 0;
  const DBCSignal *multiplexor = getMultiplexorSignal();

  if (multiplexor != nullptr) {
    // Extract multiplexor value
    multiplexValue = multiplexor->extractMultiplexorValue(data);
  }

  // Process all active signals
  std::vector<DBCSignal *> activeSignals = getActiveSignals(multiplexValue);

  for (auto &signal : activeSignals) {
    signal->processMessage(data);
  }
}

/**
 * Find the multiplexor signal if any.
 * @return The multiplexor signal or nullptr if none exists
 */
const DBCSignal *DBCMessage::getMultiplexorSignal() {
  for (auto &signal : signals) {
    if (signal->isMultiplexorSignal()) {
      return signal;
    }
  }
  return nullptr;
}

/**
 * Get all signals active for a specific multiplexor value.
 * @param multiplexValue The multiplexor value
 * @return A vector of signals active for the multiplexor value
 */
std::vector<DBCSignal *> DBCMessage::getActiveSignals(uint8_t multiplexValue) {
  std::vector<DBCSignal *> activeSignals;
  for (auto &signal : signals) {
    if (signal->isActive(multiplexValue)) {
      activeSignals.push_back(signal);
    }
  }
  return activeSignals;
}
