#ifndef SRC_DBCMESSAGE_H_
#define SRC_DBCMESSAGE_H_

#include <stdint.h>
#include <string>
#include <vector>
#include "DBCSignal.h"

class DBCMessage {
public:
  DBCMessage(uint32_t id, const std::string &name, uint8_t dlc);
  void addSignal(DBCSignal *signal);
  void processMessage(const uint8_t *data);
  
  uint32_t getId() const { return id; }
  const std::string& getName() const { return name; }
  std::vector<DBCSignal *> getSignals();
  
  // Find the multiplexor signal if any
  const DBCSignal* getMultiplexorSignal();
  
  // Get all signals active for a specific multiplexor value
  std::vector<DBCSignal *> getActiveSignals(uint8_t multiplexValue);

private:
  uint32_t id;
  std::string name;
  uint8_t dlc;  // Data Length Code
  std::vector<DBCSignal *> signals;
};

#endif