#include <FlexCAN_T4.h>

#ifndef SRC_CAN_H_
#define SRC_CAN_H_

class CAN {
    public:
        CAN();
        CAN(int BaudRate, int MaxMB, bool IsFIFO);
        
        void CAN::getStatus();
        void CAN::trackEvents();
    private:
        FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
};

#endif