#include <QNEthernet.h>
using namespace qindesign::network;

#ifndef SRC_NETWORK_H_
#define SRC_NETWORK_H_

class Network {
    public:
        Network();
        Network(IPAddress StaticIP, IPAddress SubnetMask, IPAddress Gateway);

        void    Network::registerLinkStateListener();
        uint8_t Network::startEthernet();
        void    Network::getNetworkStatus();
        void    Network::initializeUDP(uint16_t Port);


    private:
        uint16_t kPort;

        EthernetUDP udp;
        
        IPAddress staticIP;
        IPAddress subnetMask;
        IPAddress gateway;

        uint8_t macAddress[6];

};
#endif