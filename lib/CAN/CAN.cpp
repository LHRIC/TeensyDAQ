#include <CAN.h>

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
    /*if (dataFile) {
      dataFile.print(millis());
      dataFile.print(": ");
      dataFile.print(msg.id,HEX); 
      dataFile.print(", "); 
      dataFile.println(throttlePos, DEC);
      dataFile.println(); 
      dataFile.close();
    }
    */
  }
  
}

CAN::CAN() {}

CAN::CAN(int BaudRate, int MaxMB, bool IsFIFO) {
    pinMode(6, OUTPUT); 
    digitalWrite(6, LOW); /* optional tranceiver enable pin */

    Can0.begin();
    Can0.setBaudRate(BaudRate);
    Can0.setMaxMB(MaxMB);
    
    if(IsFIFO) {
        Can0.enableFIFO();
        Can0.enableFIFOInterrupt();
    }
    
    Can0.onReceive(canSniff);
}

void CAN::getStatus() {
    Can0.mailboxStatus();
}

void CAN::trackEvents() {
    Can0.events();
}