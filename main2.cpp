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
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_256> Can0; // testing new comments
SFE_UBLOX_GNSS myGNSS;
uint16_t throttlePos;

#define METADATA_SIZE 3
#define PACKET_SIZE 128
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define PNCHECK 255

#define START_FTP 0x05
unsigned long previousMillis = 0;
const long interval = 100;
int ticker = 0;
int ZIGBEE_UART_BAUDRATE = 115200;
int CHANNEL_PARAMETERS = 7;

Logger sdCard = Logger();

// Define and initialize CAN channels with IDs, byte offsets, and scalar modifiers.
// I'm so sorry for swapping between camel and snake case. It was like 3 AM.
// Channel rpm_c = Channel(0x360, 50, 0, 1, 1, 0, "rpm");
// Channel map_c = Channel(0x360, 50, 2, 3, 10, 0, "map");
// Channel tps_c = Channel(0x360, 50, 4, 5, 10, 0, "tps");
// Channel coolant_temp_c = Channel(0x3E0, 5, 0, 1, 10, -273.15, "coolant_temp");
// Channel batt_voltage_c = Channel(0x372, 10, 0, 1, 10, 0, "batt_voltage");
// Channel apps_c = Channel(0x471, 50, 2, 3, 10, 0, "apps");
// Channel o2_c = Channel(0x368, 20, 0, 1, 1000, 0, "lambda");
// Channel oil_pa_c = Channel(0x361, 50, 2, 3, 10, -101.3, "oil_pa");

// std::unordered_map<uint16_t, Channel> channelMap = {
//     {rpm_c.getChannelId(), rpm_c},
//     {map_c.getChannelId(), map_c},
//     {tps_c.getChannelId(), tps_c},
//     {coolant_temp_c.getChannelId(), coolant_temp_c},
//     {batt_voltage_c.getChannelId(), batt_voltage_c},
//     {apps_c.getChannelId(), apps_c},
//     {o2_c.getChannelId(), o2_c},
//     // {flat_shift_switch_c.getChannelId(), flat_shift_switch_c},
//     // {gear_c.getChannelId(), gear_c},
//     {oil_pa_c.getChannelId(), oil_pa_c}};

// DynamicJsonDocument doc(2048);

// const int ledPin = 13; // the number of the LED pin
// int ledState = LOW;    // ledState used to set the LED
// int inDataRecv = false;
IntervalTimer timer;

// 


// void canSniff(const CAN_message_t &msg)
// {
//   // File dataFile = SD.open("0x640data.txt", FILE_WRITE);
//   /*
//   Serial.print("MB "); Serial.print(msg.mb);
//   Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
//   Serial.print("  LEN: "); Serial.print(msg.len);
//   Serial.print(" EXT: "); Serial.print(msg.flags.extended);
//   Serial.print(" TS: "); Serial.print(msg.timestamp);
//   */
//   // Serial.print(" ID: "); Serial.print(msg.id, HEX);Serial.println();
//   /*
//   Serial.print(" Buffer: ");
//   Serial.println();

//   #define PACKET_SIZE 128
//   */

//   Serial.print("ID RECV: ");
//   Serial.print(msg.id, HEX);

//   // Find the channel that matches the incoming message.
//   Channel curChannel = channelMap[msg.id];
//   if (curChannel.getName() == "")
//   {
//     return;
//   }

//   curChannel.setValue(msg.buf);
//   curChannel.setScaledValue();
//   doc[curChannel.getName()] = curChannel.getScaledValue();

//   if (curChannel.getName() == "gear_ind_raw")
//   {
//     // gearIndication(curChannel.getScaledValue());
//   }
// }

// HELPERS
bool ft = false;
int packetNumber = 0;
bool eotStarted = false;
File ftFile;

bool xmodemChecksum(byte *packet) {
  assert(sizeof(packet) == PACKET_SIZE + METADATA_SIZE);

  int checksum = 0;
  for (int i = 0; i < PACKET_SIZE; i++) {
    checksum += (uint8_t) packet[i + METADATA_SIZE]; 
  }

  return (checksum % 256) == packet[PACKET_SIZE + METADATA_SIZE - 1];
}

void startFileTransfer() {
  ft = true;
  packetNumber = 0;
  eotStarted = false;

  ftFile = SD.open("MessageDefinitions-new.csv", FILE_WRITE);  
}

void checkForFileTransfer() {
  if (!ft) {
    byte ready[1] = {0};

    Serial.readBytes((char *) ready, 1);
    if (ready[0] == 0x01) {
    }
  }
}

void cancelFileTransfer() {
  Serial.write(CAN);
  ft = false;
  packetNumber = 0;
}

void setup(void)
{
//   pinMode(ledPin, OUTPUT);
//   digitalWrite(ledPin, HIGH);

//   pinMode(40, INPUT_PULLDOWN);

  // USB
  Serial.begin(115200);

  // Zigbee UART

//   Serial2.begin(ZIGBEE_UART_BAUDRATE);

//   Serial.println("Serial Port initialized at " + String(ZIGBEE_UART_BAUDRATE) + " baud");

  // Initialize GPS I2C

//   Wire.begin();
//   Wire.setClock(400E3);

  // myGNSS.enableDebugging();

  // while (myGNSS.begin() == false)
  // {
  //   Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
  //   delay(1000);
  // }

  // myGNSS.setI2COutput(COM_TYPE_UBX);
  // myGNSS.setVal8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, 1); // Enable L5 band for GPS

  // myGNSS.setNavigationFrequency(40); // 40 Hz

  // myGNSS.setGPSL5HealthOverride(true); // Mark L5 signals as healthy - store in RAM and BBR

  // myGNSS.softwareResetGNSSOnly(); // Restart the GNSS to apply the L5 health override

//   Serial.println("Initialized GPS I2C communication");

//   // Initialize SD Card
//   uint8_t sdInitStatus = sdCard.initialize();
//   if (sdInitStatus == 0)
//   {
//     Serial.println("SD Card initialized successfully");

//     String filename = "";

    // Wait for GPS to get a fix
    // while (myGNSS.getTimeValid() == false || myGNSS.getDateValid() == false)
    // {
    //   Serial.println("Waiting for GPS fix...");
    //   delay(1000);
    // }

    // Get the current time in Unix Epoch format
    // int32_t unixEpoch = myGNSS.getUnixEpoch();
    // filename = "log_" + String(unixEpoch) + ".txt";

    // sdCard.setFilename((char *)filename.c_str());
    // sdCard.startLogging();

    // Get MessageDefinitions.csv from the SD card
    File file = SD.open("MessageDefinitions.csv", FILE_READ);

    // if (file)
    // {
    //   Serial.println("MessageDefinitions.csv:");
    //   // Read line by line
    //   while (file.available())
    //   {
    //     String line = file.readStringUntil('\n');

    //     // Split line by comma
    //     String tokens[CHANNEL_PARAMETERS];
    //     int i = 0;
    //     int pos = 0;
    //     while (line.indexOf(',') != -1)
    //     {
    //       pos = line.indexOf(',');
    //       tokens[i] = line.substring(0, pos);
    //       line = line.substring(pos + 1);
    //       i++;
    //     }

    //     // Add channel to map
    //     Channel newChannel = Channel(tokens[0].toInt(),
    //                                  tokens[1].toInt(), tokens[2].toInt(),
    //                                  tokens[3].toInt(), tokens[4].toInt(),
    //                                  tokens[5].toInt(), tokens[6].c_str());

    //     channelMap[newChannel.getChannelId()] = newChannel;

    //     Serial.println(line);
    //   }
    //   file.close();
    // }
    // else
    // {
    //   Serial.println("Failed to open MessageDefinitions.csv");
    // }
//   }
//   else
//   {
//     Serial.println("SD Card initialization failed");
//   }

//   pinMode(6, OUTPUT);
//   digitalWrite(6, LOW);

  // Holy mother of god CAN configuration is a pain in the ass.
  // Can0.begin();

  // 1 million baud, ensure that this64 matches the ECU & Display CAN baud rate.
  // Can0.setBaudRate(1000000);

  // Max number of CAN mailboxes.
  // Can0.setMaxMB(64);
  
  // Do not use FIFO, use interrupt based CAN instead.
  // Can0.enableMBInterrupts();
  // Can0.enableFIFO();
  // Can0.enableFIFOInterrupt();
  // Can0.onReceive(canSniff);


  // Setup listeners for three mailboxe
  // Can0.onReceive(MB0, canSniff);
  // Can0.onReceive(MB1, canSniff);
  // Can0.onReceive(MB2, canSniff);

  // CAN filter setup, start by only accepting messages for these IDs.
  // Can0.setMBFilter(REJECT_ALL);
  // Can0.setMBFilter(MB0, 0x360, 0x3E0, 0x372, 0x471, 0x368);
  // Can0.enhanceFilter(MB0);
  // Can0.setMBFilter(MB1, 0x3E4, 0x470, 0x361);
  // Can0.enhanceFilter(MB1);
  // Can0.setMBFilter(MB2, 0x100);
  // Can0.enhanceFilter(MB2);

//   Can0.mailboxStatus();

//   digitalWrite(ledPin, LOW);

  // set up a timer that sends a NAK and waits for new data
  timer.begin(checkForFileTransfer, 10000000);
}


void loop()
{
  unsigned long currentMillis = millis();

  // read from serial port following XMODEM protocol
  if (ft) {
    if (Serial.available()) {
      byte receivedData[METADATA_SIZE + PACKET_SIZE] = {0};
      Serial.readBytes((char *) receivedData, METADATA_SIZE + PACKET_SIZE);

      byte _soh = receivedData[0];

      if (eotStarted) {
        if (_soh == EOT) {
          Serial.write(ACK);
          ft = false;
          eotStarted = false;
        } else {
          cancelFileTransfer();
        }
      } else {
        int _packetNumber = (int) receivedData[1];
        int _packetNumberCompliment = (int) receivedData[2];

        if ((_soh != SOH && _soh != EOT) ||
            _packetNumber != packetNumber ||
            _packetNumber + _packetNumberCompliment != PNCHECK) {
          cancelFileTransfer();
        } else if (!xmodemChecksum(receivedData)) {
          Serial.write(NAK);
        } else {
          if (_soh == SOH) {
            byte* submit = ((byte*)receivedData) + METADATA_SIZE;
            ftFile.write(submit, PACKET_SIZE);
            Serial.write(ACK);
            packetNumber++;
          } else if (_soh == EOT) {
            eotStarted = true;
            Serial.write(NAK);
            SD.remove("MessageDefinitions.csv");

            File old = SD.open("MessageDefinitions.csv", FILE_WRITE);
            ftFile.seek(0);
            old.write(ftFile.read());
            old.close();
            ftFile.close();

            SD.remove("MessageDefinitions-new.csv");
          }
        }
      }
    }
  } 
  
//   if (currentMillis - previousMillis >= interval)
//   {
//     previousMillis = currentMillis;

//     ledState = ledState == LOW ? HIGH : LOW;

//     digitalWrite(ledPin, ledState);

//     int32_t lat;
//     int32_t lon;
//     int32_t alt;
//     uint32_t epoch;
//     uint32_t microseconds;

    // Read GPS data
    // if (myGNSS.getNAVHPPOSECEF() == true)
    // {
    //   // First, let's collect the position data
    //   int32_t lat = myGNSS.getHighResECEFX();
    //   int8_t lat_hp = myGNSS.getHighResECEFXHp();
    //   int32_t lon = myGNSS.getHighResECEFY();
    //   int8_t lon_hp = myGNSS.getHighResECEFYHp();
    //   int32_t alt = myGNSS.getHighResECEFZ();
    //   int8_t alt_hp = myGNSS.getHighResECEFZHp();
    //   uint32_t accuracy = myGNSS.getPositionAccuracy();

    //   epoch = myGNSS.getUnixEpoch(microseconds);

    //   doc["lat"] = lat;
    //   doc["lat_hp"] = lat_hp;
    //   doc["lon"] = lon;
    //   doc["lon_hp"] = lon_hp;
    //   doc["alt"] = alt;
    //   doc["alt_hp"] = alt_hp;
    //   doc["accuracy"] = accuracy;
    //   doc["epoch"] = epoch;
    //   doc["us"] = microseconds;
    // }

    // Remote CAN request
    // Bytes 0-1: 11 bit CAN ID
    // Bytes 2-9: 8 bytes payload
    // byte remoteCanPayload[10] = {0};
    // while (Serial.available())
    // {
    //   Serial.readBytes((char *)remoteCanPayload, 10);

    //   CAN_message_t msg;
    //   msg.id = (remoteCanPayload[0] << 8) | remoteCanPayload[1];
    //   msg.len = 8;
    //   for (int i = 0; i < msg.len; i++)
    //   {
    //     msg.buf[i] = remoteCanPayload[i];
    //   }

    //   if (Can0.write(msg) != -1)
    //   {
    //     Serial.println("Remote CAN request sent: 0x" + String(msg.id, HEX));
    //   }
    //   else
    //   {
    //     Serial.println("Remote CAN request queued/failed: 0x" + String(msg.id, HEX));
    //   }
    // }

    // String s = String();

    // for (auto it = channelMap.begin(); it != channelMap.end(); it++)
    // {
    //   Channel curChannel = it->second;
    //   sensorMessage.set_timestamp(currentMillis);
    //   sensorMessage.set_canId(curChannel.getChannelId());
    //   // convert value to 8 bytes
    //   for (int i = 0; i < 8; i++)
    //   {
    //     sensorMessage.mutable_data()[i] = curChannel.getValue() >> (i * 8);
    //   }
    // }

    // Serial.println(s);

    // char sc[100];
    // s.toCharArray(sc, 100);
    // sdCard.println(sc);

    // char output[256] = {0};

    // serializeJson(doc, output);

    // Serialize data and send that bitch
    // sdCard.println(output);
    // Serial.println(output);
    // Serial2.println(output);

    // CAN_message_t msg;
    // msg.id = 0x789;
    // *msg.buf = ticker++;

    // if (Can0.write(msg) == -1)
    // {
    //   Serial2.println("CAN write failed?!?!?!");
    // }
    // else
    // {
    //   Serial2.println("Sent CAN message sucessfully!");
    // }
  }

//   Can0.events();
// }
