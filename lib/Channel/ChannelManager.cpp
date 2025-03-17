#include "ChannelManager.h"

void ChannelManager::addChannel(Channel channel) {
  channelMap[channel.getMessage()->getId()].push_back(channel);
  channelsByName[channel.getName()] = &channel;
}

void ChannelManager::processCANMessage(uint32_t id, const uint8_t *data) {
  auto it = channelMap.find(id);
  if (it == channelMap.end())
    return;

  for (auto &channel : it->second) {
    channel.processMessage(data);
  }
}

const std::vector<Channel> &ChannelManager::getChannelsForId(uint32_t id) const {
  auto it = channelMap.find(id);
  if (it == channelMap.end())
    return emptyVector;

  return it->second;
}

std::vector<Channel *> ChannelManager::getLoggableChannels() {
  std::vector<Channel *> loggableChannels;
  for (auto &channelPair : channelsByName) {
    if (channelPair.second->isLoggingEnabled()) {
      loggableChannels.push_back(channelPair.second);
    }
  }
  return loggableChannels;
}

std::vector<Channel *> ChannelManager::getRadioTransmitChannels() {
  std::vector<Channel *> radioTransmitChannels;
  for (auto &channelPair : channelsByName) {
    if (channelPair.second->isRadioTransmitEnabled()) {
      radioTransmitChannels.push_back(channelPair.second);
    }
  }
  return radioTransmitChannels;
}

Channel *ChannelManager::findChannelByName(const std::string &name) {
  auto it = channelsByName.find(name);
  if (it == channelsByName.end())
    return nullptr;

  return it->second;
}