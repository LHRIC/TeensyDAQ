#ifndef SRC_CHANNEL_H_
#define SRC_CHANNEL_H_

#include "DBCMessage.h"
#include "DBCSignal.h"
#include <stdint.h>
#include <string>

class Channel {
public:
  Channel(DBCSignal *signal, DBCMessage *message, const std::string displayName,
          bool log = true, bool logRaw = false, bool radioTransmit = false);

  void processMessage(const uint8_t *data);
  double getScaledValue() const { return signal->getScaledValue(); }
  double getRawValue() const { return signal->getValue(); }
  const std::string &getName() const { return displayName; }

  bool isLoggingEnabled() const { return logEnabled; }
  bool isRawLoggingEnabled() const { return logRawEnabled; }
  bool isRadioTransmitEnabled() const { return radioTransmitEnabled; }

  bool isActiveForMultiplexValue(uint8_t multiplexValue) const;

  const DBCSignal *getSignal() const { return signal; }
  const DBCMessage *getMessage() const { return message; }

private:
  DBCSignal *signal;
  DBCMessage *message;
  std::string displayName;
  bool logEnabled;
  bool logRawEnabled;
  bool radioTransmitEnabled;
};

#endif