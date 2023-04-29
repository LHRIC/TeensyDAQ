#include <Arduino.h>

#ifndef SRC_CHANNEL_H_
#define SRC_CHANNEL_H_

class Channel {
    public:
        Channel();
        Channel(uint16_t ChannelId, uint16_t SampleRate, uint8_t StartBit, uint8_t EndBit, double DivisorScalar, double AdditiveScalar);
        
        uint16_t getValue();
        void setValue(const uint8_t* buf);

        double getScaledValue();
        void setScaledValue();

        uint16_t getChannelId();


    private:
        uint16_t channelId;
        uint16_t sampleRate;
        uint8_t startBit;
        uint8_t endBit;
        uint8_t numBytes;

        double divisorScalar;
        double additiveScalar;

        uint16_t value;
        double scaledValue;

};
#endif