#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>
#include <Network.h>
#include <Channel.h>
#include <ArduinoJson-v6.21.2.h>
#include <IMU.h>
#include <Logger.h>
#include <unordered_map>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0; //testing new comments
uint16_t throttlePos;

Network network;

unsigned long previousMillis = 0;
const long interval = 100;

Logger sdCard = Logger();

// Define and initialize CAN channels with IDs, byte offsets, and scalar modifiers.
// I'm so sorry for swapping between camel and snake case. It was like 3 AM.
Channel rpm_c = Channel(0x360, 50, 0, 1, 1, 0, "rpm");
Channel map_c = Channel(0x360, 50, 2, 3, 10, 0, "map");
Channel tps_c = Channel(0x360, 50, 4, 5, 10, 0, "tps");
Channel coolant_pa_c = Channel(0x360, 50, 6, 7, 10, -101.3, "coolant_pa");
Channel coolant_temp_c = Channel(0x3E0, 5, 0, 1, 10, -273.15, "coolant_temp");
Channel batt_voltage_c = Channel(0x372, 10, 0, 1, 10, 0, "batt_voltage");
Channel apps_c = Channel(0x471, 50, 2, 3, 10, 0, "apps");
Channel o2_c = Channel(0x368, 20, 0, 1, 1000, 0, "lambda");
Channel flat_shift_switch_c = Channel(0x3E4, 5, 2, 3, 1, 0, "flat_shift_switch");
Channel upshift_request_c = Channel(0x100, 0, 0, 1, 1, 0, "upshift_request");
Channel gear_c = Channel(0x470, 20, 7, 7, 1, 0, "gear");
Channel oil_pa_c = Channel(0x361, 50, 2, 3, 10, -101.3, "oil_pa");

// Create set of channels to look up by id
// std::unordered_set<Channel> channelSet = {
//   rpm_c,
//   map_c,
//   tps_c,
//   coolant_pa_c,
//   coolant_temp_c,
//   batt_voltage_c,
//   apps_c,
//   o2_c,
//   // flat_shift_switch_c,
//   // gear_c,
//   oil_pa_c
// };

std::unordered_map<uint16_t, Channel> channelMap = {
  {rpm_c.getChannelId(), rpm_c},
  {map_c.getChannelId(), map_c},
  {tps_c.getChannelId(), tps_c},
  {coolant_pa_c.getChannelId(), coolant_pa_c},
  {coolant_temp_c.getChannelId(), coolant_temp_c},
  {batt_voltage_c.getChannelId(), batt_voltage_c},
  {apps_c.getChannelId(), apps_c},
  {o2_c.getChannelId(), o2_c},
  // {flat_shift_switch_c.getChannelId(), flat_shift_switch_c},
  // {gear_c.getChannelId(), gear_c},
  {oil_pa_c.getChannelId(), oil_pa_c}
};

// Enroll channels in channel array and channel label array
// I'm 100% sure there's a better way to do this, I was pressed for time okay.
// Channel* canChannels[] = {&rpm_c, &map_c, &tps_c, &coolant_pres_c, &coolant_temp_c, &batt_voltage_c, 
//                           &apps_c, &o2_c, &flat_shift_switch_c, &gear_c, &oil_pres};
// char* channelNames[] = {"rpm", "map", "tps", "coolant_pres", "coolant_temp", "batt_voltage", "apps", 
//                         "lambda", "oil_pres"};
// uint8_t numChannels = 11;

// Yet again, I know, far from the best way to serialize data, but we have the memory
// and I am lazy.
// const int capacity = JSON_OBJECT_SIZE();
DynamicJsonDocument doc(2048);

const int ledPin = 13;  // the number of the LED pin
int ledState = LOW;  // ledState used to set the LED

IMU imu = IMU();

bool isRecording = false;

void canSniff(const CAN_message_t &msg) {
  //File dataFile = SD.open("0x640data.txt", FILE_WRITE);
  /*
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  */
  //Serial.print(" ID: "); Serial.print(msg.id, HEX);Serial.println();
  /*
  Serial.print(" Buffer: ");
  Serial.println();
  */

  // Find the channel that matches the incoming message.
  Channel curChannel = channelMap[msg.id];
  if (curChannel.getName() == "") {
    return;
  }

  curChannel.setValue(msg.buf);
  curChannel.setScaledValue();
  doc[curChannel.getName()] = curChannel.getScaledValue();
  
  // Iterate through the channel array
  // for(int i=0; i < channel.size(); i++) {
  //   // Channel curChannel = *canChannels[i];.
  //   // Channel curChannel = *channelMap[channelNames[i]];
  //   // Check if the incoming message matches the selected channel.
  //   if(curChannel.getChannelId() == msg.id) {
  //     // If so, update the value and scale it appropriately.
  //     curChannel.setValue(msg.buf);
  //     curChannel.setScaledValue();
  //     // Additionaly, add the scaled value to the json doc.
  //     doc[channelNames[i]] = curChannel.getScaledValue();
  //   }
  // }
  
  /*
  if(msg.id == 0x100) {
    for(int i=0; i<8; i++) {
      Serial.print("AHHHHHH:< ");
      Serial.print(msg.buf[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
  */
  
}

void setup(void) {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(40, INPUT_PULLDOWN);
  
  Serial.begin(38400);


  Serial.println("Serial Port initialized at 38400 baud");
  uint8_t sdInitStatus = sdCard.initialize();
  Serial.println("Initializing Network");
  // Configure the ethernet adaptor with a static IP, subnet mask, and default gateway
  // Ensure that these settings are valid with the subnet configuration on the access point.
  IPAddress staticIp{192, 168, 0, 121}; // ip we are listening on
  IPAddress subnetMask{255, 255, 255, 0}; // amount of IP's for this device or network
  IPAddress gateway{192, 168, 0, 1}; // router ip home ip

  network = Network(staticIp, subnetMask, gateway);
  
  network.registerLinkStateListener(); 
  
  if(!network.startEthernet()) {
    Serial.println("Ethernet Failed to Start :<");
    return;
  } else {
    Serial.println("Ethernet Started Succesfully :>");
    // Initialize a UDP socket bound to port 5190.
    network.getNetworkStatus();
    network.initializeUDP(5190);
  }
  
  pinMode(6, OUTPUT); digitalWrite(6, LOW);

  // Holy mother of god CAN configuration is a pain in the ass.
  Can0.begin();
  // 1 million baud, ensure that this matches the ECU & Display CAN baud rate.
  Can0.setBaudRate(1000000);
  // Max number of CAN mailboxes. 
  Can0.setMaxMB(16);
  // Do not use FIFO, use interrupt based CAN instead.
  Can0.enableMBInterrupts();
  // Setup listeners for three mailboxe
  Can0.onReceive(MB0, canSniff);
  Can0.onReceive(MB1, canSniff);
  Can0.onReceive(MB2, canSniff);
  // CAN filter setup, start by only accepting messages for these IDs.
  // Can0.setMBFilter(REJECT_ALL);
  // Can0.setMBFilter(MB0, 0x360, 0x3E0, 0x372, 0x471, 0x368); 
  // Can0.enhanceFilter(MB0);
  // Can0.setMBFilter(MB1, 0x3E4, 0x470, 0x361); 
  // Can0.enhanceFilter(MB1);
  // Can0.setMBFilter(MB2, 0x100);
  // Can0.enhanceFilter(MB2);
  
  Can0.mailboxStatus();

  uint8_t imuStatus = imu.init();

  digitalWrite(ledPin, LOW);

//   isRecording = 1;

}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    ledState = ledState == LOW ? HIGH : LOW;

    digitalWrite(ledPin, ledState);

    // If high, start recording
    if (digitalRead(40) && !isRecording) {
      

      Serial.println("Start Recording");
      isRecording = true;
      sdCard.startLogging();

    } else if(!digitalRead(40) && isRecording){
      
      Serial.println("Stop Recording");
      isRecording = false;
    }

    if (isRecording) {
      VectorInt16 accelData = imu.getRawAccel();
      float* ypr = imu.getYPR();
      String s = String();

      s += "a/g: ";
      // s = s + ypr[0] ; 
      // s = s + ypr[1];
      // s = s + ypr[2]; 
      s = s + accelData.x / 4096.00f;
      s += ", ";
      s += accelData.y / 4096.00f;
      s += ", ";
      s += accelData.z / 4096.00f;

      

      Serial.println(s);

      char sc[100];
      s.toCharArray(sc, 100);
      sdCard.println(sc);
    }

    char output[256] = {0};


    
    //Serialize data and send that bitch
    serializeJson(doc, output);
    sdCard.println(output); 
    // serializeJson(doc, Serial);
    Serial.println(output);
    
    if(!network.sendPacket(output, 256)){
      //Serial.println("Error Sending");
    }
  }

//   imu.sample();
  Can0.events();
}


