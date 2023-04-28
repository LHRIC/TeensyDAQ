#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include <SD.h>
#include <Network.h>
#include <Channel.h>
#include <ArduinoJson-v6.21.2.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
const int chipSelect = BUILTIN_SDCARD;
uint16_t throttlePos;

Network network;

unsigned long previousMillis = 0;
const long interval = 100;

Channel rpm_c = Channel(0x360, 50, 0, 1, 1, 0);
Channel map_c = Channel(0x360, 50, 2, 3, 10, 0);
Channel tps_c = Channel(0x360, 50, 4, 5, 10, 0);
Channel coolant_pres_c = Channel(0x360, 50, 6, 7, 10, -101.3);
Channel coolant_temp_c = Channel(0x3E0, 5, 0, 1, 10, 0);
Channel batt_voltage_c = Channel(0x372, 10, 0, 1, 10, 0);
Channel apps_c = Channel(0x471, 50, 2, 3, 10, 0);

Channel canChannels[] = {rpm_c, map_c, tps_c, coolant_pres_c, coolant_temp_c, batt_voltage_c, apps_c};
uint8_t numChannels = 7;

const int capacity = JSON_OBJECT_SIZE(1);
StaticJsonDocument<capacity> doc;

void canSniff(const CAN_message_t &msg) {
  //File dataFile = SD.open("0x640data.txt", FILE_WRITE);
  /*
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  Serial.println();
  */
  /*
  for(int i=0; i < numChannels; i++) {
    if(canChannels[i].getChannelId() == msg.id) {
      //Serial.println("Got Here");
      canChannels[i].setValue(msg.buf);
      canChannels[i].setScaledValue();
    }
  }
  */
 if(msg.id == 0x471) {
  apps_c.setValue(msg.buf);
  apps_c.setScaledValue();
  //Serial.println(tps_c.getValue());
 }
  
}

void setup(void) {
  Serial.begin(9600); 
  delay(400);
  Serial.println("Hello World");
  //pinMode(PIN_D7, INPUT_PULLUP); 
  
  IPAddress staticIp{192, 168, 0, 121};
  IPAddress subnetMask{255, 255, 255, 0};
  IPAddress gateway{192, 168, 0, 1};

  network = Network(staticIp, subnetMask, gateway);
  
  network.registerLinkStateListener();
  
  if(!network.startEthernet()) {
    Serial.println("Ethernet Failed to Start :<");
    return;
  } else {
    Serial.println("Ethernet Started Succesfully :>");
    network.getNetworkStatus();
    network.initializeUDP(5190);
  }
  
  pinMode(6, OUTPUT); digitalWrite(6, LOW);

  Can0.begin();
  Can0.setBaudRate(1000000);
  Can0.setMaxMB(16);
  //Can0.enableFIFO();
  //Can0.enableFIFOInterrupt();
  Can0.enableMBInterrupts();
  Can0.onReceive(MB0, canSniff);
  Can0.setMBFilter(REJECT_ALL);
  Can0.setMBFilter(MB0, 0x360, 0x3E0, 0x372, 0x471); 
  Can0.enhanceFilter(MB0);
  Can0.mailboxStatus();
  
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    doc["tps"] = apps_c.getScaledValue();
    uint8_t output[32] = {0};
    serializeJson(doc, output);
    //Serialize data and send that bitch
    if(!network.sendPacket(output, 32)){
      Serial.println("Error Sending");
    }
  }

  Can0.events();
}