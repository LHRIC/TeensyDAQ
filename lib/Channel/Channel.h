#include <Arduino.h>

#ifndef SRC_CHANNEL_H_
#define SRC_CHANNEL_H_

class Channel {
    public:
        Channel();
        Channel(uint16_t ChannelId, uint16_t SampleRate, uint8_t StartBit, uint8_t EndBit, uint16_t DivisorScalar, int16_t AdditiveScalar);
        uint16_t getValue();
        void setValue(uint8_t* buf);

        float getScaledValue();
        void setScaledValue();


    private:
        uint16_t channelId;
        uint16_t sampleRate;
        uint8_t startBit;
        uint8_t endBit;
        uint8_t numBytes;

        uint16_t divisorScalar;
        int16_t additiveScalar;

        uint16_t value;
        float scaledValue;

};
#endif