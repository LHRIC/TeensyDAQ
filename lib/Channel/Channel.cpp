#include <Channel.h>

Channel::Channel(){};

Channel::Channel(uint16_t ChannelId, uint16_t SampleRate, uint8_t StartBit, uint8_t EndBit, 
                    double DivisorScalar, double AdditiveScalar, std::string Name, bool IsSigned, bool LittleEndian) {
    channelId = ChannelId;
    sampleRate = SampleRate;
    startBit = StartBit;
    endBit = EndBit;

    numBytes = EndBit - StartBit + 1;

    divisorScalar = DivisorScalar;
    additiveScalar = AdditiveScalar;

    name = Name;
    isSigned = IsSigned;
    littleEndian = LittleEndian;
}

uint16_t Channel::getValue(){
    return value;
}

// void Channel::setValue(const uint8_t* buf) {
//     // Assumes 16 bit Big Endian encoded data.
//     // Change to a more robust system later
//     if(startBit == endBit) {
//         value = buf[startBit];
//     } else {
//         value = (buf[startBit] << 8) | buf[endBit];
//     }
//     //Serial.println(value);
// }

void Channel::setValue(const uint8_t* buf) {
    int16_t tempValue = 0;

    if (startBit == endBit) {
        tempValue = buf[startBit];
    } else {
        if (littleEndian) {
            tempValue = (buf[endBit] << 8) | buf[startBit];
        } else {
            tempValue = (buf[startBit] << 8) | buf[endBit];
        }
    }

    if (isSigned) {
        value = static_cast<int16_t>(tempValue);
    } else {
        value = static_cast<uint16_t>(tempValue);
    }
}

double Channel::getScaledValue() {
    return scaledValue;
}

void Channel::setScaledValue() {
    scaledValue = (value / divisorScalar) + additiveScalar;
}

void Channel::setScaledValue(double value) {
    scaledValue = value;
}

uint16_t Channel::getChannelId() {
    return channelId;
}

std::string Channel::getName() {
    return name;
}

