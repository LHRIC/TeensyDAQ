#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include <Channel.h>
#include <ArduinoJson-v6.21.2.h>
#include <Logger.h>
#include <unordered_map>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <ctime>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_256> Can0; // testing new comments
SFE_UBLOX_GNSS myGNSS;
uint16_t throttlePos;
uint32_t epoch;
uint32_t nanos;
int32_t lat;
int32_t lon;
int32_t alt;

unsigned long previousMillis = 0;
const long interval = 100;
int ticker = 0;
int ZIGBEE_UART_BAUDRATE = 115200;
int CHANNEL_PARAMETERS = 7;

Logger sdCard = Logger();

// Define and initialize CAN channels with IDs, byte offsets, and scalar modifiers.
// I'm so sorry for swapping between camel and snake case. It was like 3 AM.
Channel rpm_c = Channel(0x360, 50, 0, 1, 1, 0, "rpm", false, false);
Channel map_c = Channel(0x360, 50, 2, 3, 10, 0, "map", false, false);
Channel tps_c = Channel(0x360, 50, 4, 5, 10, 0, "tps", false, false);
Channel coolant_temp_c = Channel(0x3E0, 5, 0, 1, 10, -273.15, "coolant_temp", false, false);
Channel batt_voltage_c = Channel(0x372, 10, 0, 1, 10, 0, "batt_voltage", false, false);
Channel apps_c = Channel(0x471, 50, 2, 3, 10, 0, "apps", false, false);
Channel o2_c = Channel(0x368, 20, 0, 1, 1000, 0, "lambda", false, false);
Channel oil_pa_c = Channel(0x361, 50, 2, 3, 10, -101.3, "oil_pa", false, false);
Channel gear = Channel(0x470, 20, 7, 7, 1, 0, "gear", false, false);
Channel fl_adc1 = Channel(0x404, 100, 0, 1, 1, 0, "fl_adc1", false, true);
Channel fl_adc2 = Channel(0x404, 100, 2, 3, 1, 0, "fl_adc2", false, true);
Channel fr_adc1 = Channel(0x401, 100, 0, 1, 1, 0, "fr_adc1", false, true);
Channel fr_adc2 = Channel(0x401, 100, 2, 3, 1, 0, "fr_adc2", false, true);
Channel fr_adc3 = Channel(0x401, 100, 4, 5, 1, 0, "fr_adc3", false, true);
Channel fr_adc4 = Channel(0x401, 100, 6, 7, 1, 0, "fr_adc4", false, true);
Channel rr_adc1 = Channel(0x402, 100, 0, 1, 1, 0, "rr_adc1", false, true);
Channel rr_adc2 = Channel(0x402, 100, 2, 3, 1, 0, "rr_adc2", false, true);
Channel rr_adc3 = Channel(0x402, 100, 4, 5, 1, 0, "rr_adc3", false, true);
Channel rr_adc4 = Channel(0x402, 100, 6, 7, 1, 0, "rr_adc4", false, true);
Channel cg_accel_x = Channel(0x400, 60, 0, 1, 256.0, 0, "cg_accel_x", true, false);
Channel cg_accel_y = Channel(0x400, 60, 2, 3, 256.0, 0, "cg_accel_y", true, false);
Channel cg_accel_z = Channel(0x400, 60, 4, 5, 256.0, 0, "cg_accel_z", true, false);

std::unordered_multimap<uint16_t, Channel> channelMap = {
    {rpm_c.getChannelId(), rpm_c},
    {map_c.getChannelId(), map_c},
    {tps_c.getChannelId(), tps_c},
    {coolant_temp_c.getChannelId(), coolant_temp_c},
    {batt_voltage_c.getChannelId(), batt_voltage_c},
    {apps_c.getChannelId(), apps_c},
    {o2_c.getChannelId(), o2_c},
    {gear.getChannelId(), gear},
    {oil_pa_c.getChannelId(), oil_pa_c},
    {cg_accel_x.getChannelId(), cg_accel_x},
    {cg_accel_y.getChannelId(), cg_accel_y},
    {cg_accel_z.getChannelId(), cg_accel_z},
    {fr_adc1.getChannelId(), fr_adc1},
    {fr_adc2.getChannelId(), fr_adc2},
    {fr_adc3.getChannelId(), fr_adc3},
    {fr_adc4.getChannelId(), fr_adc4},
    {rr_adc1.getChannelId(), rr_adc1},
    {rr_adc2.getChannelId(), rr_adc2},
    {rr_adc3.getChannelId(), rr_adc3},
    {rr_adc4.getChannelId(), rr_adc4}
};

DynamicJsonDocument doc(2048);

const int ledPin = 13; // the number of the LED pin
int ledState = LOW;    // ledState used to set the LED

void canSniff(const CAN_message_t &msg)
{
  // Find the channel that matches the incoming message.
  auto range = channelMap.equal_range(msg.id);
  for (auto it = range.first; it != range.second; ++it)
  {
    Channel &curChannel = it->second;
    if (curChannel.getName().empty())
    {
      continue;
    }

    curChannel.setValue(msg.buf);
    curChannel.setScaledValue();
    doc[curChannel.getName()] = curChannel.getValue();
  }
}

void gpsPVTCallback(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  std::tm timeinfo = {};
  timeinfo.tm_year = ubxDataStruct->year - 1900;
  timeinfo.tm_mon = ubxDataStruct->month - 1;
  timeinfo.tm_mday = ubxDataStruct->day;
  timeinfo.tm_hour = ubxDataStruct->hour;
  timeinfo.tm_min = ubxDataStruct->min;
  timeinfo.tm_sec = ubxDataStruct->sec;

  nanos = ubxDataStruct->nano / 1000;

  epoch = mktime(&timeinfo);
  lat = ubxDataStruct->lat;
  lon = ubxDataStruct->lon;
  alt = ubxDataStruct->hMSL;

  doc["lat"] = lat;
  doc["lon"] = lon;
  doc["alt"] = alt;
  doc["epoch"] = epoch;
  doc["nanos"] = nanos;

  // Serial.println("lat: " + String(lat) + " lon: " + String(lon) + " alt: " + String(alt) + " epoch: " + String(epoch));
}

void setup(void)
{
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(40, INPUT_PULLDOWN);

  // USB
  Serial.begin(115200);

  // Zigbee UART
  Serial2.begin(ZIGBEE_UART_BAUDRATE);

  Serial.println("Serial Port initialized at " + String(ZIGBEE_UART_BAUDRATE) + " baud");

  // Initialize GPS I2C
  Wire.begin();
  Wire.setClock(400E3);

  // myGNSS.enableDebugging();

  while (myGNSS.begin() == false)
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
    delay(1000);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX);
  myGNSS.setVal8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, 1); // Enable L5 band for GPS

  myGNSS.setNavigationFrequency(40); // 40 Hz solutions update rate

  myGNSS.setGPSL5HealthOverride(true); // Mark L5 signals as healthy - store in RAM and BBR

  myGNSS.setAutoPVTcallbackPtr(gpsPVTCallback); // Register the callback function

  myGNSS.softwareResetGNSSOnly(); // Restart the GNSS to apply the L5 health override

  Serial.println("Initialized GPS I2C communication");

  // Initialize SD Card
  uint8_t sdInitStatus = sdCard.initialize();
  if (sdInitStatus == 0)
  {
    Serial.println("SD Card initialized successfully");

    String filename = "";

    // Wait for GPS to get a fix
    while (myGNSS.getTimeValid() == false || myGNSS.getDateValid() == false)
    {
      myGNSS.checkUblox();     // Check for the arrival of new data and process it.
      myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
      Serial.println("Waiting for GPS fix...");
      delay(1000);
    }

    // Get the current time in Unix Epoch format
    int32_t unixEpoch = myGNSS.getUnixEpoch();
    filename = "log_" + String(unixEpoch) + ".txt";

    sdCard.setFilename((char *)filename.c_str());
    sdCard.startLogging();

    // Get MessageDefinitions.csv from the SD card
    // File file = SD.open("MessageDefinitions.csv", FILE_READ);

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

    //     // channelMap[newChannel.getChannelId()] = newChannel;
    //     channelMap.insert({newChannel.getChannelId(), newChannel});

    //     Serial.println(line);
    //   }
    //   file.close();
    // }
    // else
    // {
    //   Serial.println("Failed to open MessageDefinitions.csv");
    // }
  }
  else
  {
    Serial.println("SD Card initialization failed");
  }

  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);

  // Holy mother of god CAN configuration is a pain in the ass.
  Can0.begin();
  // 1 million baud, ensure that this64 matches the ECU & Display CAN baud rate.
  Can0.setBaudRate(1000000);
  // Max number of CAN mailboxes.
  Can0.setMaxMB(64);
  // Do not use FIFO, use interrupt based CAN instead.
  // Can0.enableMBInterrupts();
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(canSniff);
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

  Can0.mailboxStatus();

  digitalWrite(ledPin, LOW);

  //   isRecording = 1;
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    ledState = ledState == LOW ? HIGH : LOW;

    digitalWrite(ledPin, ledState);

    // Remote CAN request
    // Bytes 0-1: 11 bit CAN ID
    // Bytes 2-9: 8 bytes payload
    byte remoteCanPayload[10] = {0};
    while (Serial.available())
    {
      Serial.readBytes((char *)remoteCanPayload, 10);
      // Serial2.readBytes(remoteCanPayload, 10);
      CAN_message_t msg;
      msg.id = (remoteCanPayload[0] << 8) | remoteCanPayload[1];
      msg.len = 8;
      for (int i = 0; i < msg.len; i++)
      {
        msg.buf[i] = remoteCanPayload[i];
      }

      if (Can0.write(msg) != -1)
      {
        Serial.println("Remote CAN request sent: 0x" + String(msg.id, HEX));
      }
      else
      {
        Serial.println("Remote CAN request queued/failed: 0x" + String(msg.id, HEX));
      }
    }

    String s = String();

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

    // print outputs to teleplot
    for (auto it = channelMap.begin(); it != channelMap.end(); it++)
    {
      Channel curChannel = it->second;
      std::string name = ">" + curChannel.getName() + ":";
      Serial.print(name.c_str());
      Serial.println(curChannel.getScaledValue());

      Serial2.print(name.c_str());
      Serial2.println(curChannel.getScaledValue());
    }

    char output[1024] = {0};

    serializeJson(doc, output);

    // Serialize data and send that bitch
    sdCard.println(output);
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

  myGNSS.checkUblox();     // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
  Can0.events();
}
