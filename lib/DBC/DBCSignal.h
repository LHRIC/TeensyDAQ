#ifndef SRC_DBCSIGNAL_H_
#define SRC_DBCSIGNAL_H_

#include <stdint.h>
#include <string>
#include <vector>

class DBCSignal {
public:
  // Extended constructor with multiplexer information
  DBCSignal(const std::string &name, uint8_t startBit, uint8_t length, double scale, double offset,
            bool isSigned, bool isBigEndian, bool isMultiplexor = false, bool isMultiplexed = false,
            uint8_t multiplexValue = 0);

  void processMessage(const uint8_t *data);
  double getScaledValue() const;
  double getValue() const;
  const std::string &getName() const { return name; }

  // Multiplexor related methods
  bool isMultiplexorSignal() const { return isMultiplexor; }
  bool isActive(uint8_t currentMultiplexValue) const;
  uint8_t extractMultiplexorValue(const uint8_t *data) const;

private:
  std::string name;
  uint8_t startBit;
  uint8_t length;
  double scale;
  double offset;
  double value;
  double scaledValue;
  bool isSigned;
  bool isBigEndian;

  // Multiplexer related fields
  bool isMultiplexor; // True if this signal is a multiplexer
  bool isMultiplexed; // True if this signal is multiplexed
  uint8_t multiplexValue; // The value of the multiplexer signal
};

#endif