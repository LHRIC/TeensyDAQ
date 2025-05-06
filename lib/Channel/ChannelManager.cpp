#include "ChannelManager.h"
#include <Arduino.h>

/**
 * @brief Adds a copy of the given channel to the channel manager.
 * @param channel The channel to add
 */
void ChannelManager::addChannel(Channel *channel) {
  uint32_t canId = channel->getMessage()->getId();
  channelMap[canId].push_back(channel);
  channelsByName[channel->getName()] = channel;
}

void ChannelManager::processCANMessage(uint32_t id, const uint8_t *data) {
  auto it = channelMap.find(id);
  if (it == channelMap.end())
    return;

  for (auto &channel : it->second) {
    channel->processMessage(data);
  }
}

const std::vector<Channel *> ChannelManager::getChannelsForId(uint32_t id) const {
  static const std::vector<Channel *> empty;

  auto it = channelMap.find(id);
  if (it == channelMap.end())
    return empty;

  std::vector<Channel *> result;
  for (auto &channel : it->second) {
    result.push_back(channel);
  }

  return result;
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