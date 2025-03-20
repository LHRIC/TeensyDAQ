#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <TeensyThreads.h>
#include <ctime>
#include <stdint.h>

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SparkFun_u-blox_GNSS_v3.h>

#include <ChannelManager.h>
#include <DBCMessage.h>
#include <DBCSignal.h>
#include <Logger.h>
#include <MessageConfig.h>
#include <RPC.h>

#define LED_PIN 13
#define DATALOG_PIN 40

#define SERIAL_USB_BAUDRATE 115200
#define SERIAL_XBEE_BAUDRATE 115200
#define GPS_INITIALIZE_TIMEOUT_MS 5000
#define GPS_I2C_CLOCK 400E3

#define DBC_FILEPATH "/LHRDB.json"
#define SIGNAL_CONFIG_FILEPATH "/signalConfig.json"

ThreadWrap(Serial, serialUsbThreadSafe)
#define SERIAL_USB ThreadClone(serialUsbThreadSafe)

    ThreadWrap(Serial2, serialXbeeThreadSafe)
#define SERIAL_XBEE ThreadClone(serialXbeeThreadSafe)

        FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_256> Can0;

SFE_UBLOX_GNSS gnssModule;
uint32_t epoch;
uint32_t nanos;
int32_t lat;
int32_t lon;
int32_t alt;

JsonDocument doc;

Logger sdCard;

RadioReceiver xBee = RadioReceiver();

volatile uint8_t lastDatalogSwitchState = 1;
volatile uint8_t datalogSwitchState = 1;
volatile bool datalogFallingEdgeDetected = false;
volatile bool datalogRisingEdgeDetected = false;

volatile bool isLogging = false;

String filename;
volatile int32_t unixEpoch;

ChannelManager channelManager;

// Thread Co-routines
void transmitCANData() {
  while (1) {
    // Get all transmitting channels
    std::vector<Channel> radioTransmitChannels = channelManager.getRadioTransmitChannels();
    for (auto &curChannel : radioTransmitChannels) {
      std::string name = ">" + curChannel.getName() + ":";

      SERIAL_XBEE.print(name.c_str());
      SERIAL_XBEE.println(curChannel.getScaledValue());

      // Send raw data if enabled
      if (curChannel.isRawLoggingEnabled()) {
        std::string rawName = ">" + curChannel.getName() + "_raw:";
        SERIAL_XBEE.print(rawName.c_str());
        SERIAL_XBEE.println(curChannel.getRawValue());
      }
    }

    // Send GPS separately since it's not a CAN channel
    SERIAL_XBEE.print(">lat:");
    SERIAL_XBEE.println((float)doc["lat"]);
    SERIAL_XBEE.print(">long:");
    SERIAL_XBEE.println((float)doc["lon"]);

    // Yield to next thread
    threads.yield();
  }
}

void writeSDCardData() {
  uint8_t ledState = HIGH;
  while (1) {
    if (isLogging) {
      digitalWrite(LED_PIN, ledState);
      ledState = !ledState;
      char output[1024] = {0};

      serializeJson(doc, output);

      // Serialize data and send that bitch
      SERIAL_USB.println("Writing SD Data");
      sdCard.println(output, epoch, nanos / 1000);
    }
    // Log CAN map to sdCard every 100ms.
    threads.delay(100);
  }
}

void handleRXing() {
  while (1) {
    if (SERIAL_XBEE.available()) {
      char rxByte = SERIAL_XBEE.read();

      if (rxByte != -1) {
        xBee.parseFrame(rxByte);
      } else {
        threads.yield();
      }
    } else {
      threads.yield();
    }
  }
}

void monitorDatalogSwitch() {
  while (1) {
    datalogSwitchState = digitalRead(DATALOG_PIN);

    if (datalogSwitchState == 0 && lastDatalogSwitchState == 1) {
      datalogFallingEdgeDetected = true;
      datalogRisingEdgeDetected = false;
      // Start datalogging
      if (!gnssModule.getDateValid() || !gnssModule.getTimeValid()) {
        // Failed GPS Init in time
        filename = "log_millis_" + String(millis()) + ".txt";
      } else {
        // Get the current time in Unix Epoch format
        unixEpoch = gnssModule.getUnixEpoch();
        filename = "log_epoch_" + String(unixEpoch) + ".txt";
      }

      sdCard.setFilename((char *)filename.c_str());
      SERIAL_USB.print("Starting log:");
      SERIAL_USB.println(filename);
      sdCard.startLogging();
      isLogging = true;
    } else if (datalogSwitchState == 1 && lastDatalogSwitchState == 0) {
      datalogFallingEdgeDetected = false;
      datalogRisingEdgeDetected = true;
      SERIAL_USB.println("Stopping logger");
      // Stop datalogging
      sdCard.stopLogging();
      isLogging = false;
    } else {
      datalogFallingEdgeDetected = false;
      datalogRisingEdgeDetected = false;
    }
    lastDatalogSwitchState = datalogSwitchState;
    threads.delay(100);
  }
}

// ISR Callbacks
void gpsPVTCallback(UBX_NAV_PVT_data_t *ubxDataStruct) {
  std::tm timeinfo = {};
  timeinfo.tm_year = ubxDataStruct->year - 1900;
  timeinfo.tm_mon = ubxDataStruct->month - 1;
  timeinfo.tm_mday = ubxDataStruct->day;
  timeinfo.tm_hour = ubxDataStruct->hour;
  timeinfo.tm_min = ubxDataStruct->min;
  timeinfo.tm_sec = ubxDataStruct->sec;

  nanos = ubxDataStruct->nano;

  epoch = mktime(&timeinfo);
  lat = ubxDataStruct->lat;
  lon = ubxDataStruct->lon;
  alt = ubxDataStruct->hMSL;

  doc["lat"] = lat;
  doc["lon"] = lon;
  doc["alt"] = alt;
  doc["epoch"] = epoch;
  doc["nanos"] = nanos;
}

void onCANMessageCallback(const CAN_message_t &msg) {
  // Process the CAN message
  channelManager.processCANMessage(msg.id, msg.buf);

  // Log signals in the message
  if (isLogging) {
    std::vector<Channel> channels = channelManager.getChannelsForId(msg.id);
    for (auto &curChannel : channels) {
      std::string name = curChannel.getName();
      double value = curChannel.getScaledValue();
      double rawValue = curChannel.getRawValue();

      doc[name.c_str()] = value;

      SERIAL_USB.print(name.c_str());
      SERIAL_USB.print(": ");
      SERIAL_USB.println(value);

      // Log raw data if enabled
      if (curChannel.isRawLoggingEnabled()) {
        std::string rawName = name + "_raw";
        doc[rawName.c_str()] = rawValue;
      }
    }
  }
}

// Initial Setup
void setupGPS() {
  while (gnssModule.begin() == false) {
    SERIAL_USB.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
    delay(1000);
  }

  gnssModule.setI2COutput(COM_TYPE_UBX);

  // Enable L5 band for GPS
  gnssModule.setVal8(UBLOX_CFG_SIGNAL_GPS_L5_ENA, 1);

  // 40 Hz solutions update rate
  gnssModule.setNavigationFrequency(40);

  // Mark L5 signals as healthy - store in RAM and BBR
  gnssModule.setGPSL5HealthOverride(true);

  // Register the callback function
  gnssModule.setAutoPVTcallbackPtr(gpsPVTCallback);

  // Restart the GNSS to apply the L5 health override
  gnssModule.softwareResetGNSSOnly();

  SERIAL_USB.println("Initialized GPS I2C communication");
}

void waitForGPSFix(uint32_t msTimeout) {
  // Wait for GPS to get a fix (timeout after 15s)
  uint32_t startGPSInitTime = millis();
  while ((gnssModule.getTimeValid() == false || gnssModule.getDateValid() == false) &&
         (millis() - startGPSInitTime <= msTimeout)) {
    // Check for the arrival of new data and process it.
    gnssModule.checkUblox();

    // Check if any callbacks are waiting to be processed.
    gnssModule.checkCallbacks();

    SERIAL_USB.println("Waiting for GPS fix...");
    delay(500);
  }
}

void setupCANBus() {
  Can0.begin();
  Can0.setBaudRate(1E6);
  Can0.setMaxMB(64);

  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(onCANMessageCallback);

  // Can0.mailboxStatus();
}

void setupSDCard() {
  uint8_t sdInitStatus = sdCard.initialize();
  if (sdInitStatus == 0) {
    SERIAL_USB.println("SD Card initialized successfully");
  } else {
    SERIAL_USB.println("SD Card initialization failed");
  }
}

void setupChannels() {
  // Load channel map from JSON files
  std::unordered_map<uint32_t, std::vector<Channel>> channelMap =
      MessageConfigParser::getChannelMap(SIGNAL_CONFIG_FILEPATH, DBC_FILEPATH);

  // Add channels to the channel manager
  for (auto it = channelMap.begin(); it != channelMap.end(); it++) {
    for (Channel curChannel : it->second) {
      channelManager.addChannel(curChannel);
    }
  }
}

void setup(void) {
  // Setup GPIO
  pinMode(LED_PIN, OUTPUT);
  pinMode(DATALOG_PIN, INPUT);
  digitalWrite(LED_PIN, HIGH);

  // UART Channel Setup
  SERIAL_USB.begin(SERIAL_USB_BAUDRATE);
  SERIAL_XBEE.begin(SERIAL_XBEE_BAUDRATE);

  // Intialize I2C Bus for GPS communication
  Wire.begin();
  Wire.setClock(GPS_I2C_CLOCK);

  // Setup Channels
  setupChannels();

  // Initialize GPS module
  setupGPS();
  waitForGPSFix(GPS_INITIALIZE_TIMEOUT_MS);

  setupSDCard();

  setupCANBus();

  // Dispatch threads
  // threads.addThread(handleRXing);
  threads.addThread(transmitCANData);
  threads.addThread(monitorDatalogSwitch);

  // Allocate 4KB of stack memory for this thread.
  // Any less and you risk causing a stack overflow,
  // which will cause the Teensy to hard fault.
  //
  // Don't ask me how I know...
  threads.addThread(writeSDCardData, 0, 4096);

  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // Check for the arrival of new data and process it.
  gnssModule.checkUblox();

  // Check if any callbacks are waiting to be processed.
  gnssModule.checkCallbacks();

  // Check FlexCAN event callbacks available
  Can0.events();
}
