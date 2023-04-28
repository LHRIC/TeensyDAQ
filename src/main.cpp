#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include <SD.h>
#include <Network.h>
#include <CAN.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
const int chipSelect = BUILTIN_SDCARD;
uint16_t throttlePos;

Network network;
CAN can;

void setup(void) {
  Serial.begin(115200); 
  delay(400);
  //pinMode(PIN_D7, INPUT_PULLUP); 
  IPAddress staticIp{192, 168, 0, 121};
  IPAddress subnetMask{255, 255, 255, 0};
  IPAddress gateway{192, 168, 0, 1};

  network = Network(staticIp, subnetMask, gateway);
  network.registerLinkStateListener();
  if(!network.startEthernet()) {
    Serial.print("Ethernet Failed to Start :<");
    return;
  } else {
    network.getNetworkStatus();
    network.initializeUDP(5190);
  }

  can = CAN(1000000, 16, true);
  can.getStatus();
}

void loop() {
  can.trackEvents();
}