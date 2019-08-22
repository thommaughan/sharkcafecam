/*
 Adpated code. When run, prints all possible readings for IMU.
 */

#include <SPI.h>    // uSD
#include <SD.h>
#include <Wire.h>   // i2c
#include "quaternionFilters.h"
#include "MPU9250.h"


void setup() {
  // put your setup code here, to run once:
//MPU9250
#define I2Cclock 400000
#define I2Cport Wire
#define MPU9250_ADDRESS MPU9250_ADDRESS_AD0
float imu_srate = 100.0;
}

void loop() {
  // put your main code here, to run repeatedly:
 /////MPU9250////////MPU9250///////MPU9250//////MPU9250////////
    MPU9250 myIMU0(MPU9250_ADDRESS, I2Cport, I2Cclock);
    MPU9250 myIMU1(MPU9250_ADDRESS, I2Cport, I2Cclock);
    myIMU0.initMPU9250();
//    myIMU0.MPU9250SelfTest(myIMU0.selfTest);
//    Serial.print(F("x-axis self test: acceleration trim within : "));
//    Serial.print(myIMU0.selfTest[0],1); Serial.println("% of factory value");
//    Serial.print(F("y-axis self test: acceleration trim within : "));
//    Serial.print(myIMU0.selfTest[1],1); Serial.println("% of factory value");
//    Serial.print(F("z-axis self test: acceleration trim within : "));
//    Serial.print(myIMU0.selfTest[2],1); Serial.println("% of factory value");
//    Serial.print(F("x-axis self test: gyration trim within : "));
//    Serial.print(myIMU0.selfTest[3],1); Serial.println("% of factory value");
//    Serial.print(F("y-axis self test: gyration trim within : "));
//    Serial.print(myIMU0.selfTest[4],1); Serial.println("% of factory value");
//    Serial.print(F("z-axis self test: gyration trim within : "));
//    Serial.print(myIMU0.selfTest[5],1); Serial.println("% of factory value");
//    delay(1000);
    // Calibrate gyro and accelerometers, load biases in bias registers
    myIMU0.calibrateMPU9250(myIMU0.gyroBias, myIMU0.accelBias);
    myIMU0.readAccelData(myIMU0.accelCount);
  
  myIMU0.initMPU9250();
  byte c = 0x00;
  byte d = 0x00;
  c = myIMU0.readByte(MPU9250_ADDRESS_AD0, WHO_AM_I_MPU9250);
  d = myIMU1.readByte(MPU9250_ADDRESS_AD1, WHO_AM_I_MPU9250);
  //test comm with MPU
//  Serial.print(F("MPU9250 I AM 0x"));
//  Serial.print(c, HEX);
//  Serial.print(F(" I should be 0x"));
//  Serial.println(0x71, HEX);
//  Serial.print("Received AD0: 0x");
//  Serial.print(c, HEX);
//  Serial.print(", AD1: 0x");
//  Serial.println(d, HEX);
    //read gyro, accel, and magnet
    myIMU0.getAres();
    myIMU0.getGres();
    myIMU0.getMres();
    //accel
    myIMU0.readAccelData(myIMU0.accelCount);
    // This depends on scale being set
    myIMU0.ax = (float)myIMU0.accelCount[0] * myIMU0.aRes; // - myIMU.accelBias[0];
    myIMU0.ay = (float)myIMU0.accelCount[1] * myIMU0.aRes; // - myIMU.accelBias[1];
    myIMU0.az = (float)myIMU0.accelCount[2] * myIMU0.aRes; // - myIMU.accelBias[2];
//    Serial.println(myIMU0.ax);
//    Serial.println(myIMU0.ay);
//    Serial.println(myIMU0.az);
    
    //Gyro
    myIMU0.readGyroData(myIMU0.gyroCount);  // Read the x/y/z adc values
    // Calculate the gyro value into actual degrees per second
    // This depends on scale being set
    myIMU0.gx = (float)myIMU0.gyroCount[0] * myIMU0.gRes;
    myIMU0.gy = (float)myIMU0.gyroCount[1] * myIMU0.gRes;
    myIMU0.gz = (float)myIMU0.gyroCount[2] * myIMU0.gRes;
//    Serial.println(myIMU0.gx);
//    Serial.println(myIMU0.gy);
//    Serial.println(myIMU0.gz);

    //magnetometer
    myIMU0.readMagData(myIMU0.magCount);  // Read the x/y/z adc values

    // Calculate the magnetometer values in milliGauss
    // Include factory calibration per data sheet and user environmental
    // corrections
    // Get actual magnetometer value, this depends on scale being set
    myIMU0.mx = (float)myIMU0.magCount[0] * myIMU0.mRes
               * myIMU0.factoryMagCalibration[0] - myIMU0.magBias[0];
    myIMU0.my = (float)myIMU0.magCount[1] * myIMU0.mRes
               * myIMU0.factoryMagCalibration[1] - myIMU0.magBias[1];
    myIMU0.mz = (float)myIMU0.magCount[2] * myIMU0.mRes
               * myIMU0.factoryMagCalibration[2] - myIMU0.magBias[2];
               
    // Get magnetometer calibration from AK8963 ROM
    myIMU0.initAK8963(myIMU0.factoryMagCalibration);
    // Initialize device for active mode read of magnetometer
    //Serial.println("AK8963 initialized for active data mode....");
    delay(100);
  ///////////////////////////////////////////////////////
   // IMU
  myIMU0.updateTime();
  MahonyQuaternionUpdate(myIMU0.ax, myIMU0.ay, myIMU0.az, myIMU0.gx * DEG_TO_RAD,
                         myIMU0.gy * DEG_TO_RAD, myIMU0.gz * DEG_TO_RAD, myIMU0.my,
                         myIMU0.mx, myIMU0.mz, myIMU0.deltat);
  // Print acceleration values in milligs!
  Serial.print("X-acceleration: "); Serial.print(1000 * myIMU0.ax);
  Serial.print(" mg ");
  Serial.print("Y-acceleration: "); Serial.print(1000 * myIMU0.ay);
  Serial.print(" mg ");
  Serial.print("Z-acceleration: "); Serial.print(1000 * myIMU0.az);
  Serial.println(" mg ");

  // Print gyro values in degree/sec
  Serial.print("X-gyro rate: "); Serial.print(myIMU0.gx, 3);
  Serial.print(" degrees/sec ");
  Serial.print("Y-gyro rate: "); Serial.print(myIMU0.gy, 3);
  Serial.print(" degrees/sec ");
  Serial.print("Z-gyro rate: "); Serial.print(myIMU0.gz, 3);
  Serial.println(" degrees/sec");

  // Print mag values in degree/sec
  Serial.print("X-mag field: "); Serial.print(myIMU0.mx);
  Serial.print(" mG ");
  Serial.print("Y-mag field: "); Serial.print(myIMU0.my);
  Serial.print(" mG ");
  Serial.print("Z-mag field: "); Serial.print(myIMU0.mz);
  Serial.println(" mG");

  myIMU0.tempCount = myIMU0.readTempData();  // Read the adc values
  // Temperature in degrees Centigrade
  myIMU0.temperature = ((float) myIMU0.tempCount) / 333.87 + 21.0;
  // Print temperature in degrees Centigrade
  Serial.print("Temperature is ");  Serial.print(myIMU0.temperature, 1);
  Serial.println(" degrees C");
}
