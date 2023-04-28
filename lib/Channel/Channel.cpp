#include <Channel.h>

Channel::Channel(){};

Channel::Channel(uint16_t ChannelId, uint16_t SampleRate, uint8_t StartBit, uint8_t EndBit, uint16_t DivisorScalar, int16_t AdditiveScalar) {
    channelId = channelId;
    sampleRate = SampleRate;
    startBit = StartBit;
    endBit = EndBit;

    numBytes = EndBit - StartBit + 1;

    divisorScalar = DivisorScalar;
    additiveScalar = AdditiveScalar;
}

uint16_t Channel::getValue( ){
    return value;
}

void Channel::setValue(uint8_t* buf) {
    // Assumes 16 bit Big Endian encoded data.
    // Change to a more robust system later
    value = (buf[startBit] << 8) | buf[endBit];
}

float Channel::getScaledValue() {
    return scaledValue;
}

void Channel::setScaledValue() {
    scaledValue = (value / divisorScalar) + additiveScalar;
}