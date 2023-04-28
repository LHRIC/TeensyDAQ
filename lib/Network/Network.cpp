#include <Network.h>
#include <Arduino.h>
using namespace qindesign::network;

Network::Network() {}

Network::Network(IPAddress StaticIP, IPAddress SubnetMask, IPAddress Gateway) {
    staticIP = StaticIP;
    subnetMask = SubnetMask;
    gateway = Gateway;

    Ethernet.macAddress(macAddress);  // This is informative; it retrieves, not sets
    Serial.printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
         macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    
    
}

void Network::registerLinkStateListener() {
    Ethernet.onLinkState([](bool state) {
        Serial.printf("[Ethernet] Link %s\r\n", state ? "ON" : "OFF");
    });
}

uint8_t Network::startEthernet() {
    uint8_t status = Ethernet.begin(staticIP, subnetMask, gateway);
    return status;
}

void Network::getNetworkStatus() {
  IPAddress ip = Ethernet.localIP();
  Serial.printf("    Local IP     = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.subnetMask();
  Serial.printf("    Subnet mask  = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.broadcastIP();
  Serial.printf("    Broadcast IP = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.gatewayIP();
  Serial.printf("    Gateway      = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.dnsServerIP();
  Serial.printf("    DNS          = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
}

void Network::initializeUDP(uint16_t Port) {
    kPort = Port;
    udp.begin(kPort);
}

uint8_t Network::sendPacket(uint8_t * buf, uint16_t bufLen) {
    uint8_t status = udp.send(Ethernet.broadcastIP(), kPort,
            reinterpret_cast<const uint8_t *>(buf),
            bufLen);
    return status;
}
