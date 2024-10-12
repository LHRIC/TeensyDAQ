#include <Arduino.h>
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

#define METADATA_SIZE 3
#define PACKET_SIZE 128
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define PNCHECK 255

#define START_FTP 0x05

Logger sdCard = Logger();

bool ft = false;
int packetNumber = 0;
bool eotStarted = false;
File ftFile;

long interval = 100;
long previousMillis;

IntervalTimer timer;

bool xmodemChecksum(byte *packet)
{
    assert(sizeof(packet) == PACKET_SIZE + METADATA_SIZE);

    int checksum = 0;
    for (int i = 0; i < PACKET_SIZE; i++)
    {
        checksum += (uint8_t)packet[i + METADATA_SIZE];
    }

    return (checksum % 256) == packet[PACKET_SIZE + METADATA_SIZE - 1];
}

void startFileTransfer()
{
    // ft = true;
    packetNumber = 0;
    eotStarted = false;

    ftFile = SD.open("MessageDefinitions-new.csv", FILE_WRITE);
}

void checkForFileTransfer()
{
    if (!ft)
    {
        byte ready[1] = {0};

        Serial.readBytes((char *)ready, 1);
        if (ready[0] == 0x01)
        {
        }
    }
}

void cancelFileTransfer()
{
    // Serial.println("Canceling file transfer");
    Serial.write(CAN);
    ft = false;
    packetNumber = 0;
}

void setup()
{
    Serial.begin(115200); // Ensure this matches the Raspberry Pi's baud rate
    while (!Serial)
        ; // Wait for the Serial to initialize
    // Serial.println("Teensy!");
    File file = SD.open("MessageDefinitions.csv", FILE_READ);
    startFileTransfer();
    // timer.begin(checkForFileTransfer, 10000000);
}

boolean handlePacket()
{
    while (Serial.available() >= METADATA_SIZE + PACKET_SIZE)
    {
        byte receivedData[METADATA_SIZE + PACKET_SIZE] = {0};
        Serial.readBytes((char *)receivedData, METADATA_SIZE + PACKET_SIZE);

        // Serial.println("receivedData");
        byte _soh = receivedData[0];

        // for (int i = 0; i < METADATA_SIZE + PACKET_SIZE; i++) {
        //     Serial.println(receivedData[i], HEX);
        // }

        if (eotStarted)
        {
            if (_soh == EOT)
            {
                Serial.write(ACK);
                ft = false;
                eotStarted = false;
            }
            else
            {
                cancelFileTransfer();
                return 1;
            }
        }
        else
        {
            int _packetNumber = (int)receivedData[1];
            int _packetNumberCompliment = (int)receivedData[2];

            if ((_soh != SOH && _soh != EOT) ||
                _packetNumber != packetNumber ||
                _packetNumber + _packetNumberCompliment != PNCHECK)
            {
                cancelFileTransfer();
                return 1;
            }
            else if (!xmodemChecksum(receivedData))
            {
                Serial.write(NAK);
            }
            else
            {
                if (_soh == SOH)
                {
                    byte *submit = ((byte *)receivedData) + METADATA_SIZE;
                    ftFile.write(submit, PACKET_SIZE);
                    Serial.write(ACK);
                    packetNumber++;
                }
                else if (_soh == EOT)
                {
                    eotStarted = true;
                    Serial.write(NAK);
                    SD.remove("MessageDefinitions.csv");

                    File old = SD.open("MessageDefinitions.csv", FILE_WRITE);
                    ftFile.seek(0);
                    old.write(ftFile.read());
                    old.close();
                    ftFile.close();

                    SD.remove("MessageDefinitions-new.csv");
                    return 0;
                }
            }
        }
    }

    return 1;
}

void xmodem()
{
    // Serial.println("Now running xmodem protocol");
    while (handlePacket() != 1);
    ft = false;
}

void loop()
{
    unsigned long currentMillis = millis();

    if (!ft)
    {
        while (Serial.available() > 0)
        {
            uint8_t code = Serial.read();
         
            if (code == START_FTP)
            {
                Serial.write(code);
                xmodem();
                break;
            }
        }
    }
    // read from serial port following XMODEM protocol
}
