#include <Arduino.h>
#include <queue>
#include <vector>

#ifndef SRC_RPC_H_
#define SRC_RPC_H_

// Delimiters
#define SOF_BYTE 0x02
#define EOF_BYTE 0x03

// Command IDs
#define COMMAND_CAN_BROADCAST 0xFA

#define RX_BUFFER_LEN 8

/*
Message Header Specification
(Byte Index)
     0-1: SOF
     1-2: COMMAND_ID
     2-4: PAYLOAD_LEN (bytes)
     ...: <DATA_PAYLOAD>
 n-2 - n: CRC-16
       n: EOF
*/

enum RXState {
    WAIT_SOF,
    READ_LEN,
    READ_CID,
    READ_PAYLOAD,
    READ_EOF,
};

class RadioReceiver {
    public:
        RadioReceiver();
        
        void parseFrame(char byte);
    private:

        RXState rxState;
        char rxBuffer[RX_BUFFER_LEN] = {0};
        std::vector<char> payload;

        uint16_t readLen = 0x0000;
        uint8_t commandId;
};

#endif