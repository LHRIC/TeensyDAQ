#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include <SD.h>
#include <Network.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
const int chipSelect = BUILTIN_SDCARD;
uint16_t throttlePos;

Network network;

void canSniff(const CAN_message_t &msg) {
  //File dataFile = SD.open("0x640data.txt", FILE_WRITE);
  
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  Serial.println();
  
  /*
  
  if(msg.id == 0x360) {
    for(int i=0; i < msg.len; i++){
      Serial.print(msg.buf[i]);
      Serial.print(", ");  
    }
    Serial.println();
    uint16_t throttlePos = (msg.buf[4] << 8) | msg.buf[5];
    Serial.println(throttlePos * 0.1); 
    //Serial.print(msg.buf[msg.len - 2]);
    //Serial.print(",");
    //Serial.println(msg.buf[msg.len - 1]);
  }
  */
  
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
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(canSniff);
  Can0.mailboxStatus();
  
}

void loop() {
  Can0.events();
}