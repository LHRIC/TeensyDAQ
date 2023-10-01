#ifndef SRC_IMU2_H_
#define SRC_IMU2_H_

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

class IMU2 {
    public:
        IMU2();

        uint8_t IMU2::init2(TwoWire *Wire);
        uint8_t IMU2::initNoDMP2();
        VectorInt16 IMU2::getAccel2();
        VectorInt16 IMU2::getRawAccel2();
        VectorInt16 IMU2::getRawGyro2();
        float* IMU2::getYPR2();
        void IMU2::zero2();
        
    private:
        // MPU control/status vars
        bool dmpReady2;  // set true if DMP init was successful
        uint8_t mpuIntStatus2;   // holds actual interrupt status byte from MPU
        uint8_t devStatus2;      // return status after each device operation (0 = success, !0 = error)
        uint16_t packetSize2;    // expected DMP packet size (default is 42 bytes)
        uint16_t fifoCount2;     // count of all bytes currently in FIFO
        uint8_t fifoBuffer2[64]; // FIFO storage buffer

        // orientation/motion vars
        Quaternion q2;           // [w, x, y, z]         quaternion container
        VectorInt16 aa2;         // [x, y, z]            accel sensor measurements
        VectorInt16 aaReal2;     // [x, y, z]            gravity-free accel sensor measurements
        VectorInt16 aaWorld2;    // [x, y, z]            world-frame accel sensor measurements
        VectorInt16 aaRaw2;      // [x, y, z]            raw accel sensor measurements
        VectorFloat gravity2;    // [x, y, z]            gravity vector
        float euler2[3];         // [psi, theta, phi]    Euler angle container
        float ypr2[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector


};

#endif