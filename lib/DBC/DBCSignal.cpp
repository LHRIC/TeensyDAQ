#include "DBCSignal.h"

DBCSignal::DBCSignal(const std::string &name, uint8_t startBit, uint8_t length, double scale,
                     double offset, bool isSigned, bool isBigEndian, bool isMultiplexor,
                     bool isMultiplexed, uint8_t multiplexValue)
    : name(name), startBit(startBit), length(length), scale(scale), offset(offset), value(0),
      scaledValue(0), isSigned(isSigned), isBigEndian(isBigEndian), isMultiplexor(isMultiplexor),
      isMultiplexed(isMultiplexed), multiplexValue(multiplexValue) {}

/**
 * Correctly process a message using the DBC bit numbering convention.
 * @param data The raw message data
 */
void DBCSignal::processMessage(const uint8_t *data) {
  uint64_t rawValue = 0;

  if (isBigEndian) {
    // For big-endian (Motorola) signals with DBC bit numbering

    // Find the start byte and bit position
    uint8_t startByte = startBit / 8;
    uint8_t bitPosition = startBit % 8;

    // Calculate bit positions
    uint8_t currentByte = startByte;
    uint8_t currentBit = bitPosition;
    uint8_t bitsRemaining = length;

    while (bitsRemaining > 0) {
      // Process bits from MSB to LSB order (left to right in each byte)
      // How many bits can we read from current byte
      uint8_t bitsToRead = 1;

      // Extract bits from current byte
      uint8_t extractedBits = (data[currentByte] & 0x01) >> currentBit;

      // Extract the current bit
      uint8_t bitValue = (data[currentByte] >> currentBit) & 0x01;

      // Add this bit to our result (shifting left for big-endian order)
      rawValue = (rawValue << 1) | bitValue;

      bitsRemaining--;

      // Move to next bit (going left to right within byte)
      if (currentBit == 0) {
        // We've reached the end of this byte, go to next byte
        currentByte++;
        currentBit = 7; // Start at MSB of next byte
      } else {
        // Move to next bit position to the right
        currentBit--;
      }
    }
  } else {
    // Find start byte and bit position
    uint8_t startByte = startBit / 8;
    uint8_t bitPosition = startBit % 8;
    uint8_t bitsRemaining = length;
    uint8_t bitShift = 0; // Track position in result

    // Process bytes from start position
    while (bitsRemaining > 0) {
      // How many bits we can read from current byte
      uint8_t bitsToRead = 8 - bitPosition;
      if (bitsToRead > bitsRemaining) {
        bitsToRead = bitsRemaining;
      }

      // Create mask for the bits we want
      uint8_t mask = ((1 << bitsToRead) - 1) << bitPosition;

      // Extract bits from current byte and add to result
      uint8_t extractedBits = (data[startByte] & mask) >> bitPosition;
      rawValue |= (uint64_t)extractedBits << bitShift;

      // Update counters
      bitsRemaining -= bitsToRead;
      bitShift += bitsToRead;
      startByte++;     // Move to next byte
      bitPosition = 0; // Start from bit 0 in next byte
    }
  }

  // Process signed values if needed
  if (isSigned) {
    uint64_t signBit = 1ULL << (length - 1);
    if (rawValue & signBit) {
      uint64_t mask = ~((1ULL << length) - 1);
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
  // If this is the multiplexor signal or is not multiplexed, it's always active
  if (isMultiplexor || !isMultiplexed) {
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
