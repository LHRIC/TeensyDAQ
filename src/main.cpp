#include <Arduino.h>
#include <TeensyThreads.h>
#include <FlexCAN_T4.h>
#include <ctime>
#include <unordered_map>

#include <ArduinoJson-v6.21.2.h>
#include <EEPROM.h>
#include <SparkFun_u-blox_GNSS_v3.h>

#include <Channel.h>
#include <Logger.h>
#include <RPC.h>

#define LED_PIN 13
#define DATALOG_PIN 40

#define SERIAL_USB_BAUDRATE 115200
#define SERIAL_XBEE_BAUDRATE 115200
#define GPS_INITIALIZE_TIMEOUT_MS 5000
#define GPS_I2C_CLOCK 400E3

ThreadWrap(Serial, serialUsbThreadSafe)
#define SERIAL_USB ThreadClone(serialUsbThreadSafe)

ThreadWrap(Serial2, serialXbeeThreadSafe)
#define SERIAL_XBEE ThreadClone(serialXbeeThreadSafe)

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_256> Can0; // testing new comments

SFE_UBLOX_GNSS gnssModule;
uint32_t epoch;
uint32_t nanos;
int32_t lat;
int32_t lon;
int32_t alt;

DynamicJsonDocument doc(2048);

Logger sdCard;

RadioReceiver xBee = RadioReceiver();

// ECU channels
Channel rpm_c = Channel(0x360, 50, 0, 1, 1, 0, "rpm", false, false);
Channel map_c = Channel(0x360, 50, 2, 3, 10, 0, "map", false, false);
Channel tps_c = Channel(0x360, 50, 4, 5, 10, 0, "tps", false, false);
Channel coolant_temp_c = Channel(0x3E0, 5, 0, 1, 10, -273.15, "coolant_temp", false, false);
Channel batt_voltage_c = Channel(0x372, 10, 0, 1, 10, 0, "batt_voltage", false, false);
Channel apps_c = Channel(0x471, 50, 2, 3, 10, 0, "apps", false, false);
Channel o2_c = Channel(0x368, 20, 0, 1, 1000, 0, "lambda", false, false);
Channel oil_pa_c = Channel(0x361, 50, 2, 3, 10, -101.3, "oil_pa", false, false);
Channel gear = Channel(0x470, 20, 7, 7, 1, 0, "gear", false, false);

// CAN board channels
Channel cg_accel_x = Channel(0x400, 60, 0, 1, 256.0, 0, "cg_accel_x", true, true);
Channel cg_accel_y = Channel(0x400, 60, 2, 3, 256.0, 0, "cg_accel_y", true, true);
Channel cg_accel_z = Channel(0x400, 60, 4, 5, 256.0, 0, "cg_accel_z", true, true);
Channel fl_adc1 = Channel(0x404, 100, 0, 1, 1, 0, "fl_adc1", false, false);
Channel fl_adc2 = Channel(0x404, 100, 2, 3, 1, 0, "fl_adc2", false, false);
Channel fl_adc3 = Channel(0x404, 100, 4, 5, 1, 0, "fl_adc3", false, false);
Channel fl_adc4 = Channel(0x404, 100, 6, 7, 1, 0, "fl_adc4", false, false);
Channel fr_adc1 = Channel(0x401, 100, 0, 1, 1, 0, "fr_adc1", false, false);
Channel fr_adc2 = Channel(0x401, 100, 2, 3, 1, 0, "fr_adc2", false, false);
Channel fr_adc3 = Channel(0x401, 100, 4, 5, 1, 0, "fr_adc3", false, false);
Channel fr_adc4 = Channel(0x401, 100, 6, 7, 1, 0, "fr_adc4", false, false);
Channel rr_adc1 = Channel(0x402, 100, 0, 1, 1, 0, "rr_adc1", false, false);
Channel rr_adc2 = Channel(0x402, 100, 2, 3, 1, 0, "rr_adc2", false, false);
Channel rr_adc3 = Channel(0x402, 100, 4, 5, 1, 0, "rr_adc3", false, false);
Channel rr_adc4 = Channel(0x402, 100, 6, 7, 1, 0, "rr_adc4", false, false);

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

volatile uint8_t lastDatalogSwitchState = 1;
volatile uint8_t datalogSwitchState = 1;
volatile bool datalogFallingEdgeDetected = false;
volatile bool datalogRisingEdgeDetected = false;

volatile bool isLogging = false;

String filename;
volatile int32_t unixEpoch;

// Thread Co-routines
void transmitCANData() {
  while (1) {
    for (auto it = channelMap.begin(); it != channelMap.end(); it++) {
      Channel curChannel = it->second;
      std::string name = ">" + curChannel.getName() + ":";

      SERIAL_XBEE.print(name.c_str());
      SERIAL_XBEE.println(curChannel.getScaledValue());
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
    if(isLogging) {
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
  while(1) {
    if(SERIAL_XBEE.available()) {
      char rxByte = SERIAL_XBEE.read();

      if(rxByte != -1){
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
  while(1) {
    datalogSwitchState = digitalRead(DATALOG_PIN);
    
    if(datalogSwitchState == 0 && lastDatalogSwitchState == 1) {
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

  //Can0.mailboxStatus();
}

void setupSDCard() {
  uint8_t sdInitStatus = sdCard.initialize();
  if (sdInitStatus == 0) {
    SERIAL_USB.println("SD Card initialized successfully");
  } else {
    SERIAL_USB.println("SD Card initialization failed");
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

  setupGPS();
  waitForGPSFix(GPS_INITIALIZE_TIMEOUT_MS);

  setupSDCard();

  setupCANBus();

  // Dispatch threads
  //threads.addThread(handleRXing);
  
  threads.addThread(transmitCANData);
  threads.addThread(monitorDatalogSwitch);
  
  // Allocate 4MB of stack memory for this thread.
  // Any less and you risk causing a stack overflow, 
  // which will cause the Teensy to hard fault.
  //
  // Don't ask me how I know...
  threads.addThread(writeSDCardData, 0, 4096);

  threads.addThread(handleRXing);

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
