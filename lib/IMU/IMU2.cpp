#include <IMU2.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "MPU6050_6Axis_MotionApps20.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

#define LED_PIN 13
MPU6050 mpu2(0x68, &Wire1);
volatile bool mpuInterrupt2;

void dmpDataReady2(){
    mpuInterrupt2 = true;
}

IMU2::IMU2(){
    dmpReady2 = false;
    mpuInterrupt2 = false;
   
};

// uint8_t IMU2::initNoDMP() {
//      // join I2C bus (I2Cdev library doesn't do this automatically)
//     #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
//         Wire.begin();
//     #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
//         Fastwire::setup(400, true);
//     #endif

//     // initialize serial communication
//     // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
//     // it's really up to you depending on your project)
//     Serial.begin(38400);

//     // initialize device
//     Serial.println("Initializing I2C devices...");
//     mpu2.initialize();

//     // verify connection
//     Serial.println("Testing device connections...");
//     Serial.println(mpu2.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

//     // use the code below to change accel/gyro offset values
//     /*
//     Serial.println("Updating internal sensor offsets...");
//     // -76	-2359	1688	0	0	0
//     Serial.print(accelgyro.getXAccelOffset()); Serial.print("\t"); // -76
//     Serial.print(accelgyro.getYAccelOffset()); Serial.print("\t"); // -2359
//     Serial.print(accelgyro.getZAccelOffset()); Serial.print("\t"); // 1688
//     Serial.print(accelgyro.getXGyroOffset()); Serial.print("\t"); // 0
//     Serial.print(accelgyro.getYGyroOffset()); Serial.print("\t"); // 0
//     Serial.print(accelgyro.getZGyroOffset()); Serial.print("\t"); // 0
//     Serial.print("\n");
//     accelgyro.setXGyroOffset(220);
//     accelgyro.setYGyroOffset(76);
//     accelgyro.setZGyroOffset(-85);
//     Serial.print(accelgyro.getXAccelOffset()); Serial.print("\t"); // -76
//     Serial.print(accelgyro.getYAccelOffset()); Serial.print("\t"); // -2359
//     Serial.print(accelgyro.getZAccelOffset()); Serial.print("\t"); // 1688
//     Serial.print(accelgyro.getXGyroOffset()); Serial.print("\t"); // 0
//     Serial.print(accelgyro.getYGyroOffset()); Serial.print("\t"); // 0
//     Serial.print(accelgyro.getZGyroOffset()); Serial.print("\t"); // 0
//     Serial.print("\n");
//     */

//     // configure Arduino LED pin for output
//     pinMode(LED_PIN, OUTPUT);
// }

uint8_t IMU2::init2(TwoWire *bus){
    Serial.printf("MPU6050 with I2C bus %p initializing...", bus);
    // // These if statements dont work for some reason, HAVE NO IDEA WHY???
    // Serial.printf("SDA: %d SCL: %d\n", SDA, SCL);
    // if (SDA == 18 && SCL == 19) {
    //     Wire.begin();
    // } else if (SDA == 17 && SCL == 16) {
    //     Wire1.begin();
    // } else if (SDA == 25 && SCL == 24) {
    //     Wire2.begin();
    // } 
    
    

    // Wire.setClock(400000);

    mpu2.initialize();
    Serial.println(mpu2.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
    Serial.println(F("Initializing DMP..."));
    devStatus2 = mpu2.dmpInitialize();

    mpu2.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);
    mpu2.setFullScaleGyroRange(MPU6050_GYRO_FS_500);
    

    if (devStatus2 == 0) {
        // Calibration Time: generate offsets and calibrate our MPU6050
        mpu2.CalibrateAccel(6);
        mpu2.CalibrateGyro(6);
        mpu2.PrintActiveOffsets();
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu2.setDMPEnabled(true);

        // enable Arduino interrupt detection
        Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
        Serial.print(digitalPinToInterrupt(2));
        Serial.println(F(")..."));
        attachInterrupt(digitalPinToInterrupt(2), dmpDataReady2, RISING);
        mpuIntStatus2 = mpu2.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady2 = true;

        // get expected DMP packet size for later comparison
        packetSize2 = mpu2.dmpGetFIFOPacketSize();

        return 0;
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus2);
        Serial.println(F(")"));
        return devStatus2;
    }
};

VectorInt16 IMU2::getAccel2(){
    if (!dmpReady2) return;
    // read a packet from FIFO
    if (mpu2.dmpGetCurrentFIFOPacket(fifoBuffer2)) { // Get the Latest packet 
        // display Euler angles in degrees
        /*
        mpu2.dmpGetQuaternion(&q, fifoBuffer);
        mpu2.dmpGetGravity(&gravity, &q);
        mpu2.dmpGetYawPitchRoll(ypr, &q, &gravity);
        Serial.print("ypr\t");
        Serial.print(ypr[0] * 180/M_PI);
        Serial.print("\t");
        Serial.print(ypr[1] * 180/M_PI);
        Serial.print("\t");
        Serial.println(ypr[2] * 180/M_PI);
        */

        mpu2.dmpGetQuaternion(&q2, fifoBuffer2);
        mpu2.dmpGetAccel(&aa2, fifoBuffer2);
        mpu2.dmpGetGravity(&gravity2, &q2);
        mpu2.dmpGetLinearAccel(&aaReal2, &aa2, &gravity2);
        mpu2.dmpGetLinearAccelInWorld(&aaWorld2, &aaReal2, &q2);
        
        // Serial.print("aworld\t");
        // Serial.print(aaWorld.x);
        // Serial.print("\t");
        // Serial.print(aaWorld.y);
        // Serial.print("\t");
        // Serial.println(aaWorld.z);

        return aaReal2;
    }
};

VectorInt16 IMU2::getRawAccel2() {   
    // if(!dmpReady) return;

    mpu2.getAcceleration(&aaRaw2.x, &aaRaw2.y, &aaRaw2.z);

    return aaRaw2;
}

VectorInt16 IMU2::getRawGyro2() {
    VectorInt16 gyro;

    mpu2.getRotation(&gyro.x, &gyro.y, &gyro.z);

    return gyro;
}

float* IMU2::getYPR2(){
    if (!dmpReady2) return;
    // read a packet from FIFO
    if (mpu2.dmpGetCurrentFIFOPacket(fifoBuffer2)) { // Get the Latest packet 
        // display Euler angles in degrees
        
        mpu2.dmpGetQuaternion(&q2, fifoBuffer2);
        mpu2.dmpGetGravity(&gravity2, &q2);
        mpu2.dmpGetYawPitchRoll(ypr2, &q2, &gravity2);
        Serial.print("ypr\t");
        Serial.print(ypr2[0] * 180/M_PI);
        Serial.print("\t");
        Serial.print(ypr2[1] * 180/M_PI);
        Serial.print("\t");
        Serial.println(ypr2[2] * 180/M_PI);

        return ypr2;
    }
}





void IMU2::zero2(){
    mpu2.CalibrateAccel(6);
    mpu2.CalibrateGyro(6);
    mpu2.PrintActiveOffsets();
}