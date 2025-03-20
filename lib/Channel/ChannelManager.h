#ifndef SRC_CHANNEL_MANAGER_H_
#define SRC_CHANNEL_MANAGER_H_

#include "Channel.h"
#include "DBCMessage.h"
#include <string>
#include <unordered_map>
#include <vector>

class ChannelManager {
public:
  void addChannel(Channel &channel);

  void processCANMessage(uint32_t id, const uint8_t *data);

  const std::vector<Channel> getChannelsForId(uint32_t id) const;

  std::vector<Channel> getLoggableChannels();

  std::vector<Channel> getRadioTransmitChannels();

  Channel *findChannelByName(const std::string &name);

private:
  std::unordered_map<uint32_t, std::vector<Channel>> channelMap;

  std::unordered_map<std::string, Channel*> channelsByName;

  // Static empty vector to return when no channels are found
  const std::vector<Channel> emptyVector;
};

#endif