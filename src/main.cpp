#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <ctime>
#include <unordered_map>

#include <ArduinoJson-v6.21.2.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <TeensyThreads.h>

#include <Channel.h>
#include <Logger.h>

#define SERIAL_USB Serial
#define SERIAL_XBEE Serial2

#define LED_PIN 13
#define DATALOG_PIN 40

#define SERIAL_USB_BAUDRATE 115200
#define SERIAL_XBEE_BAUDRATE 115200
#define GPS_INITIALIZE_TIMEOUT_MS 15000
#define GPS_I2C_CLOCK 400E3

Threads::Mutex usbSerialLock;
Threads::Mutex xBeeSerialLock;

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_256> Can0; // testing new comments

SFE_UBLOX_GNSS gnssModule;
uint32_t epoch;
uint32_t nanos;
int32_t lat;
int32_t lon;
int32_t alt;

DynamicJsonDocument doc(2048);

Logger sdCard = Logger();

// ECU channels
Channel rpm_c = Channel(0x360, 50, 0, 1, 1, 0, "rpm", false, false);
Channel map_c = Channel(0x360, 50, 2, 3, 10, 0, "map", false, false);
Channel tps_c = Channel(0x360, 50, 4, 5, 10, 0, "tps", false, false);
Channel coolant_temp_c =
    Channel(0x3E0, 5, 0, 1, 10, -273.15, "coolant_temp", false, false);
Channel batt_voltage_c =
    Channel(0x372, 10, 0, 1, 10, 0, "batt_voltage", false, false);
Channel apps_c = Channel(0x471, 50, 2, 3, 10, 0, "apps", false, false);
Channel o2_c = Channel(0x368, 20, 0, 1, 1000, 0, "lambda", false, false);
Channel oil_pa_c = Channel(0x361, 50, 2, 3, 10, -101.3, "oil_pa", false, false);
Channel gear = Channel(0x470, 20, 7, 7, 1, 0, "gear", false, false);

// CAN board channels
Channel cg_accel_x =
    Channel(0x400, 60, 0, 1, 256.0, 0, "cg_accel_x", true, true);
Channel cg_accel_y =
    Channel(0x400, 60, 2, 3, 256.0, 0, "cg_accel_y", true, true);
Channel cg_accel_z =
    Channel(0x400, 60, 4, 5, 256.0, 0, "cg_accel_z", true, true);
Channel fl_adc1 = Channel(0x404, 100, 0, 1, 1, 0, "fl_adc1", false, true);
Channel fl_adc2 = Channel(0x404, 100, 2, 3, 1, 0, "fl_adc2", false, true);
Channel fl_adc3 = Channel(0x404, 100, 4, 5, 1, 0, "fl_adc3", false, true);
Channel fl_adc4 = Channel(0x404, 100, 6, 7, 1, 0, "fl_adc4", false, true);
Channel fr_adc1 = Channel(0x401, 100, 0, 1, 1, 0, "fr_adc1", false, true);
Channel fr_adc2 = Channel(0x401, 100, 2, 3, 1, 0, "fr_adc2", false, true);
Channel fr_adc3 = Channel(0x401, 100, 4, 5, 1, 0, "fr_adc3", false, true);
Channel fr_adc4 = Channel(0x401, 100, 6, 7, 1, 0, "fr_adc4", false, true);
Channel rr_adc1 = Channel(0x402, 100, 0, 1, 1, 0, "rr_adc1", false, true);
Channel rr_adc2 = Channel(0x402, 100, 2, 3, 1, 0, "rr_adc2", false, true);
Channel rr_adc3 = Channel(0x402, 100, 4, 5, 1, 0, "rr_adc3", false, true);
Channel rr_adc4 = Channel(0x402, 100, 6, 7, 1, 0, "rr_adc4", false, true);

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
    {rr_adc4.getChannelId(), rr_adc4},
    {fl_adc1.getChannelId(), fl_adc1},
    {fl_adc2.getChannelId(), fl_adc2},
    {fl_adc3.getChannelId(), fl_adc3},
    {fl_adc4.getChannelId(), fl_adc4}};

// Thread Co-routines
void readRadioCommands() {
  while (1) {
    // Attempt to acquire mutex
    while (xBeeSerialLock.try_lock() == 0) {
      threads.yield();
    }

    // Default timeout is 1000ms, change with SERIAL_XBEE.setTimeout()
    char commandBuffer[10] = {0};
    SERIAL_XBEE.readBytesUntil('\n', commandBuffer, 10);

    // Do whatever with the buffer
    CAN_message_t msg;
    msg.id = (commandBuffer[0] << 8) | commandBuffer[1];
    msg.len = 8;
    for (int i = 0; i < msg.len; i++) {
      msg.buf[i] = commandBuffer[i + 2];
    }

    Can0.write(msg);

    // Release the mutex
    xBeeSerialLock.unlock();

    // Yield to the next thread
    threads.yield();
  }
}

void transmitCANData() {
  while (1) {
    // Attempt to acquire mutex
    while (xBeeSerialLock.try_lock() == 0) {
      threads.yield();
    }

    for (auto it = channelMap.begin(); it != channelMap.end(); it++) {
      Channel curChannel = it->second;
      std::string name = ">" + curChannel.getName() + ":";

      SERIAL_XBEE.print(name.c_str());
      SERIAL_XBEE.println(curChannel.getScaledValue());

      SERIAL_XBEE.print(">lat:");
      SERIAL_XBEE.println((float)doc["lat"]);
      SERIAL_XBEE.print(">long:");
      SERIAL_XBEE.println((float)doc["lon"]);
    }

    threads.yield();
  }
}

void writeSDCardData() {
  while (1) {
    char output[1024] = {0};

    serializeJson(doc, output);

    // Serialize data and send that bitch
    sdCard.println(output);

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
}

void onCANMessageCallback(const CAN_message_t &msg) {
  // Find the channel that matches the incoming message.
  auto range = channelMap.equal_range(msg.id);
  for (auto it = range.first; it != range.second; ++it) {
    Channel &curChannel = it->second;
    if (curChannel.getName().empty()) {
      continue;
    }

    curChannel.setValue(msg.buf);
    curChannel.setScaledValue();
    doc[curChannel.getName()] = curChannel.getValue();
  }
}

// Initial Setup
void setupGPS() {
  while (gnssModule.begin() == false) {
    SERIAL_USB.println(
        F("u-blox GNSS not detected at default I2C address. Retrying..."));
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
  while ((gnssModule.getTimeValid() == false ||
          gnssModule.getDateValid() == false) &&
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

  Can0.mailboxStatus();
}

void setupSDCard() {
  uint8_t sdInitStatus = sdCard.initialize();
  String filename;
  int32_t unixEpoch;
  if (sdInitStatus == 0) {
    SERIAL_USB.println("SD Card initialized successfully");

    if (!gnssModule.getDateValid() || !gnssModule.getTimeValid()) {
      // Failed GPS Init in time
      filename = "log_" + String(millis()) + ".txt";
    } else {
      // Get the current time in Unix Epoch format
      unixEpoch = gnssModule.getUnixEpoch();
      filename = "log_" + String(unixEpoch) + ".txt";
    }

    sdCard.setFilename((char *)filename.c_str());
    sdCard.startLogging();
  } else {
    SERIAL_USB.println("SD Card initialization failed");
  }
}

void setup(void) {
  // Setup GPIO
  pinMode(LED_PIN, OUTPUT);
  pinMode(DATALOG_PIN, INPUT_PULLDOWN);
  digitalWrite(LED_PIN, HIGH);

  // UART Channel Setup
  SERIAL_USB.begin(SERIAL_USB_BAUDRATE);
  SERIAL_XBEE.begin(SERIAL_XBEE_BAUDRATE);

  // Intialize I2C Bus for GPS communication
  Wire.begin();
  Wire.setClock(GPS_I2C_CLOCK);

  setupGPS();
  waitForGPSFix(GPS_INITIALIZE_TIMEOUT_MS);

  setupSDCard();

  setupCANBus();

  // Dispatch threads
  threads.addThread(readRadioCommands);
  threads.addThread(transmitCANData);
  threads.addThread(writeSDCardData);

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
