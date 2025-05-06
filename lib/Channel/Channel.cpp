#include "Channel.h"

Channel::Channel(DBCSignal *signal, DBCMessage *message, const std::string displayName,
                 bool log, bool logRaw, bool radioTransmit)
    : signal(signal), message(message), displayName(displayName), logEnabled(log),
      logRawEnabled(logRaw), radioTransmitEnabled(radioTransmit) {}

void Channel::processMessage(const uint8_t *data) {
  message->processMessage(data);
}

bool Channel::isActiveForMultiplexValue(uint8_t multiplexValue) const {
  return signal ? signal->isActive(multiplexValue) : false;
}