#include "DBCSignal.h"

DBCSignal::DBCSignal(const std::string &name, uint8_t startBit, uint8_t length, double scale,
                     double offset, bool isSigned, bool isBigEndian, bool isMultiplexor,
                     bool isMultiplexed, uint8_t multiplexValue)
    : name(name), startBit(startBit), length(length), scale(scale), offset(offset), value(0),
      scaledValue(0), isSigned(isSigned), isBigEndian(isBigEndian), isMultiplexor(isMultiplexor),
      isMultiplexed(isMultiplexed), multiplexValue(multiplexValue) {}

/**
 * Process a message and extract the signal value.
 * @param data The raw message data
 */
void DBCSignal::processMessage(const uint8_t *data) {
  uint64_t rawValue = 0;
  int bitPos = startBit;

  if (isBigEndian) {
    // Motorola format (Big Endian)
    for (int i = 0; i < length; i++) {
      int byteIndex = 7 - (bitPos / 8);
      int bitIndex = bitPos % 8;

      if (data[byteIndex] & (1 << bitIndex)) {
        rawValue |= (uint64_t)1 << i;
      }

      bitPos--;
      if (bitPos < 0)
        bitPos = 63; // wrap around
    }
  } else {
    // Intel format (Little Endian)
    for (int i = 0; i < length; i++) {
      int byteIndex = bitPos / 8;
      int bitIndex = bitPos % 8;

      if (data[byteIndex] & (1 << bitIndex)) {
        rawValue |= (uint64_t)1 << i;
      }

      bitPos++;
    }
  }

  // Convert raw value to signed if necessary
  if (isSigned) {
    // Handle sign extension
    int signBit = 1 << (length - 1);
    if (rawValue & signBit) {
      // Negative value, extend sign bit
      uint64_t mask = 0xFFFFFFFFFFFFFFFFULL << length;
      rawValue |= mask;
    }
    value = static_cast<double>(static_cast<int64_t>(rawValue));
  } else {
    value = static_cast<double>(rawValue);
  }

  // Apply scaling and offset
  scaledValue = value * scale + offset;
}

/**
 * Get the scaled value of the signal.
 * @return The scaled value of the signal
 */
double DBCSignal::getScaledValue() const { return scaledValue; }

/**
 * Get the raw value of the signal.
 * @return The raw value of the signal
 */
double DBCSignal::getValue() const { return value; }

/**
 * Check if multiplexor signal is active for the given multiplexor value.
 * @param currentMultiplexValue The current multiplexor value
 * @return True if the multiplexor signal is active for the given multiplexor value, false otherwise
 */
bool DBCSignal::isActive(uint8_t currentMultiplexValue) const {
  // If this is the multiplexor signal, it's always active
  if (isMultiplexor) {
    return true;
  }

  // If this signal is not multiplexed, it's always active
  if (!isMultiplexed) {
    return true;
  }

  // If this signal is multiplexed, it's active only when the
  // multiplexor has the right value
  return multiplexValue == currentMultiplexValue;
}

uint8_t DBCSignal::extractMultiplexorValue(const uint8_t *data) const {
  // Process message to get the value
  DBCSignal temp(*this);
  temp.processMessage(data);

  // Return the raw value as the multiplexor value
  return static_cast<uint8_t>(temp.getValue());
}