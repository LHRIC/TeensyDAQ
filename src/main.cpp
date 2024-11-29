#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include <Channel.h>
#include <ArduinoJson-v6.21.2.h>
#include <Logger.h>
#include <cassert>
#include <filesystem>
#include <cstdio>
#include <unordered_map>
#include <SparkFun_u-blox_GNSS_v3.h>

#define FRONT_METADATA_SIZE 5
#define BACK_METADATA_SIZE 1
#define PACKET_SIZE 128
#define TOTAL_PACKET_SIZE FRONT_METADATA_SIZE + PACKET_SIZE + BACK_METADATA_SIZE
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define STATUS_INCOMPLETE 0x24
#define STATUS_ACK 0x25
#define STATUS_NAK 0x26
#define STATUS_FAILURE 0x27
#define STATUS_COMPLETE 0x28

#define PACKET_NUMBER_CHECK 255

#define START_FTP 0x05

Logger sdCard = Logger();

bool ft = false;
int packetNumber = 0;
bool eotStarted = false;
File ftFile;


long interval = 100;
long previousMillis;

void clearSerialBuffer() {
    Serial.flush();
    
    while (Serial.available() > 0) {
        Serial.read();
    }
}

bool checksum(byte *packet)
{
    int checksum = 0;
    for (int i = 0; i < PACKET_SIZE; i++)
    {
        checksum += (uint8_t)packet[i + FRONT_METADATA_SIZE];
    }

    return (checksum % 256) == packet[TOTAL_PACKET_SIZE - 1];
}

void startFileTransfer()
{
    packetNumber = 0;
    eotStarted = false;

    ftFile = SD.open("MessageDefinitions-new.csv", FILE_WRITE);
}

void resetFileTransfer()
{
    ft = false;
    eotStarted = false;
    packetNumber = 0;

    clearSerialBuffer();
}

void setup()
{
    Serial.begin(115200); // Ensure this matches the Raspberry Pi's baud rate
    while (!Serial); // Wait for the Serial to initialize

    File file = SD.open("MessageDefinitions.csv", FILE_READ);
    startFileTransfer();
}

int handlePacket()
{
    if (Serial.available() >= TOTAL_PACKET_SIZE)
    {
        byte receivedData[TOTAL_PACKET_SIZE] = {0};
        Serial.readBytes((char *)receivedData, TOTAL_PACKET_SIZE);

        byte header = receivedData[0];

        if (eotStarted)
        {
            if (header == EOT)
            {
                ft = false;
                eotStarted = false;
                SD.remove("MessageDefinitions.csv");

                File old = SD.open("MessageDefinitions.csv", FILE_WRITE);
                ftFile.seek(0);
                old.write(ftFile.read());
                old.close();
                ftFile.close();

                SD.remove("MessageDefinitions-new.csv");

                return STATUS_COMPLETE;
            }
            else
            {
                return STATUS_FAILURE;
            }
        }
        else
        {
            int _higher_order_packet_number = (int)receivedData[1];
            int _lower_order_packet_number = (int)receivedData[2];
            int _higher_order_packet_number_compliment = (int)receivedData[3];
            int _lower_order_packet_number_compliment = (int)receivedData[4];

            int _packetNumber = (_higher_order_packet_number << 8) + (_lower_order_packet_number);

            if ((header != SOH && header != EOT) ||
                _packetNumber != packetNumber ||
                _higher_order_packet_number + _higher_order_packet_number_compliment != PACKET_NUMBER_CHECK ||
                _lower_order_packet_number + _lower_order_packet_number_compliment != PACKET_NUMBER_CHECK)
            {
                if (_packetNumber != packetNumber) {
                    Serial.write("F");
                }
                if (_higher_order_packet_number + _higher_order_packet_number_compliment != PACKET_NUMBER_CHECK) {
                    Serial.write("G");
                }
                if (_lower_order_packet_number + _lower_order_packet_number_compliment != PACKET_NUMBER_CHECK) {
                    Serial.write("H");
                }
                return STATUS_FAILURE;
                
            }
            else if (!checksum(receivedData))
            {
                return STATUS_NAK;
            }
            else
            {
                if (header == SOH)
                {
                    byte *submit = (byte *)receivedData;
                    ftFile.write(submit, PACKET_SIZE);
                    return STATUS_ACK;
                }
                else if (header == EOT)
                {
                    eotStarted = true;
                    packetNumber++;
                    return STATUS_NAK;
                }
            }
        }
    } else {
        return STATUS_INCOMPLETE;
    }
}

void xmodem()
{
    while (1) {
        int code = STATUS_INCOMPLETE;

        // wait until we have a full packet of data
        while ((code = handlePacket()) == STATUS_INCOMPLETE);

        switch (code) {
            case STATUS_FAILURE:
                Serial.write(CAN);
                resetFileTransfer();
                return;
            case STATUS_ACK:
                Serial.write(ACK);
                packetNumber += 1;
                break;
            case STATUS_NAK:
                Serial.write(NAK);
                break;
            case STATUS_COMPLETE:
                Serial.write(EOT);
                resetFileTransfer();
                return;
            default:
                // something went very wrong
                Serial.write(CAN);
                resetFileTransfer();
                return;
        }
    }
    
}

void loop()
{
    if (!ft)
    {
        while (Serial.available() > 0)
        {
            uint8_t code = Serial.read();

            if (code == START_FTP)
            {
                Serial.write(code);
                ft = true;
                clearSerialBuffer();
                xmodem();
                break;
            }
        }
    }
}
