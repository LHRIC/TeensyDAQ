#include <TeensyThreads.h>
#include <RPC.h>

ThreadWrap(Serial, serialUsbThreadSafe2)
#define SERIAL_USB ThreadClone(serialUsbThreadSafe2)

RadioReceiver::RadioReceiver() {
    rxState = WAIT_SOF;
};

// Co-routine to recieve UART data from the radio
void RadioReceiver::parseFrame(char byte) {
    switch(rxState) {
        case WAIT_SOF:
            if (byte == SOF_BYTE) {
                // Clear the rxBuffer and proceed to READ_LEN
                rxState = READ_CID;
                
                SERIAL_USB.println("SOF Detected");
            }
            break;
        case READ_CID:
            commandId = (uint8_t) byte;
            rxState = READ_LEN;

            SERIAL_USB.print("CID: ");
            SERIAL_USB.print(commandId, HEX);
            SERIAL_USB.println();

            // Ensure that payload is clear
            payload.clear();
            break;
        case READ_LEN:
            payload.push_back(byte);

            if(payload.size() == 2) {
                readLen = (payload[0] << 8) | payload[1];
                payload.clear();

                if(readLen == 0) {
                    rxState = WAIT_SOF;
                } else {
                    rxState = READ_PAYLOAD;
                }

                SERIAL_USB.print("Len: ");
                SERIAL_USB.print(readLen, HEX);
                SERIAL_USB.println();
            }
            break;
        case READ_PAYLOAD:
            payload.push_back(byte);
            if (payload.size() >= readLen){
                rxState = READ_EOF;

                SERIAL_USB.print("Data: ");
                for(const char& i : payload) {
                    SERIAL_USB.print(i);
                }
                SERIAL_USB.println();
            }
            break;
        case READ_EOF:
            if(byte == EOF_BYTE) {
                SERIAL_USB.println("EOF Detected");
                SERIAL_USB.println("====");
            } else {
                // No EOF, smthng weird happened
            }
            rxState = WAIT_SOF;
            break;
        default:
            break;
    }
};
