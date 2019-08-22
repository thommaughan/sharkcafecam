#include <SPI.h>    // uSD
#include <SD.h>
#include "wiring_private.h" //pinPeripheral() function for serial2 pin mux
#include <Wire.h>   // i2c
#include "SFE_ISL29125.h"
#include "quaternionFilters.h"
#include "MPU9250.h"
#include "amx32.h"
//#include "LowPower.h"

//style guide
// ALL CAPS names are used for 'defines' - this gives names to numbers.
// define names for the IO pins used, try to use schematic names or something close, 
// make other defines that map those schematic names to something more 'logical' as needed
// in programming practice, these gives a symbolic name for a number and solves the problem
// of 'magic numbers'.  it also makes it so the number can be changed in once place (in 
// the 'define') rather than all of the code.  
// An example is the led on pin 13.   
//


#define BOARDTYPE 1   // 0 means Feather M0, 1 means thomz dwsTag

#if BOARDTYPE == 1
#define LED1            26      //26 //PA27  { PORTA, 27, PIO_TIMER, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_15 }, // p39 previously TX LED
#define D10_SD_CS       10      //10 //PA18 { PORTA, 18, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM3_CH0, TC3_CH0, EXTERNAL_INT_2 }, // p27
#define A2_CURBAT       16      //16 //PB9 { PORTB,  9, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel3, PWM4_CH1, TC4_CH1, EXTERNAL_INT_9 }, // p8
#define A4_VBAT         18      //18 //PA5 { PORTA,  5, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel5, PWM0_CH1, TCC0_CH1, EXTERNAL_INT_5 }, // p10
#define D8_PERIPH_VREG_EN  8     //8  //PA6  { PORTA,  6, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel6, PWM1_CH0, TCC1_CH0, EXTERNAL_INT_6 }, // p11
#define D9_IO_PWR       9       //9  //PA7  { PORTA,  7, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel7, PWM1_CH1, TCC1_CH1, EXTERNAL_INT_7 }, // p12
#define D4_IO_REC       4       //4 //PA8  { PORTA,  8, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel16, PWM0_CH0, TCC0_CH0, EXTERNAL_INT_NMI }, // p23
#define VBATSW_EN       3       //3  //PA9  { PORTA,  9, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel17, PWM0_CH1, TCC0_CH1, EXTERNAL_INT_9 }, // p14
#define LO_CONT         6       //6  //PA20 { PORTA, 20, PIO_TIMER_ALT, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER_ALT), No_ADC_Channel, PWM0_CH6, TCC0_CH6, EXTERNAL_INT_4 }, // p29
#define LO_REL          7       //7  //PA21 { PORTA, 21, PIO_TIMER_ALT, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER_ALT), No_ADC_Channel, PWM0_CH7, TCC0_CH7, EXTERNAL_INT_5 }, // p30
#define HI_REL          19      //19 //PB2 { PORTB,  2, PIO_ANALOG, 0, ADC_Channel10, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_2 }, // p47
#define D13_IMU_INT     13      //13 //PA17 { PORTA, 17, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM2_CH1, TCC2_CH1, EXTERNAL_INT_1 }, // p26 SCK
#define VCONTMON        17      //V1.8 hardware not routed, 17 //PA4 { PORTA,  4, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel4, PWM0_CH0, TCC0_CH0, EXTERNAL_INT_4 }, // p9
#define SERCOM1_RX      12      // atsamd21g sercom1 peripheral is mapped to software Serial2 object
#define SERCOM1_TX      11      // atsamd21g sercom1 peripheral

//MPU9250
#define I2Cclock 400000
#define I2Cport Wire
#define MPU9250_ADDRESS MPU9250_ADDRESS_AD0
float imu_srate = 100.0;

//RGB
SFE_ISL29125 RGB_sensor;//RGB sensor 
#define LEDRED      LED1
#define SD_CS       D10_SD_CS
#endif
//-------------------------------

File logfile;

// RX on pin 12 which is portA on the processor PA19, TX on pin 11 PA16
Uart Serial2 (&sercom1, SERCOM1_RX, SERCOM1_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);

void setup()
{
  gpio_init();            // dswTag or Feather M0 handled here
  Wire.begin();
  //connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  //also spit it out
  Serial.begin(115200);
  //while(!Serial)    {;}

  Serial2.begin(115200);
  // while(!Serial2)    {;}
  
  //Assign pins 12 & 11 SERCOM functionality
  pinPeripheral(SERCOM1_RX, PIO_SERCOM); // Serial2 RX  12
  pinPeripheral(SERCOM1_TX, PIO_SERCOM); // Serial2 TX  11
  delay(2000);

  //see if the card is present and can be initialized:
  if(!SD.begin(SD_CS))
  {
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "/ANALOG00.TXT");
  for (uint8_t i = 0; i < 100; i++)
  {
    filename[7] = '0' + i/10;
    filename[8] = '0' + i%10;
//create if does not exist, do not open existing, write, sync after write
    if(! SD.exists(filename))
    {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if(!logfile)
  {
    Serial.print("Couldnt create ");
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to ");
  Serial.println(filename);

  Serial.println("Ready! test1");
}



void gpio_init()
{
#if BOARDTYPE == 0
  pinMode(LEDRED, OUTPUT);    // 13
  pinMode(8, OUTPUT);
#endif

  
#if BOARDTYPE == 1  
    pinMode(D8_PERIPH_VREG_EN, OUTPUT);     //8  //PA6
    digitalWrite(D8_PERIPH_VREG_EN, HIGH);   // voltage regulator for FCD1004 (if left off, causes i2c hang)
    pinMode(VBATSW_EN, OUTPUT);             //3  //PA9
    digitalWrite(VBATSW_EN, LOW);
    pinMode(LED1, OUTPUT);            //26 //PA27// appLed (was ledGreen on iTag, is redLed on dwstag)
    digitalWrite(LED1, LOW);
    pinMode(LO_CONT, OUTPUT);        //6  //PA20  // LO_CONT PA20
    digitalWrite(LO_CONT, LOW);  
    pinMode(LO_REL, OUTPUT);          //7  //PA21
    digitalWrite(LO_REL, LOW);       // LO_REL PA21
    pinMode(HI_REL, OUTPUT);          //19 //PB2
    digitalWrite(HI_REL, LOW);        // aka BURN (burn is used on schematic for iTag compat)

    //new
    pinMode(D10_SD_CS, OUTPUT);     //10 //PA18// used for sd card
    digitalWrite(D10_SD_CS, LOW);
    pinMode(D9_IO_PWR, OUTPUT);   //9  //PA7
    digitalWrite(D9_IO_PWR, LOW);
    pinMode(D4_IO_REC, OUTPUT);          //4 //PA8
    digitalWrite(D4_IO_REC, LOW);

    pinMode(D13_IMU_INT, INPUT);      //13 //PA17   Not used?? 
        
    // PB9, pin 8, A2 is CURBAT  BatteryCurrent
    pinMode(A2_CURBAT, INPUT);
    pinMode(A4_VBAT, INPUT);  
    pinMode(VCONTMON, INPUT);    // A3 //PA4
    pinMode(A4_VBAT, INPUT);        //18 //PA5

     //Test RGB initialization
      if (RGB_sensor.init()){
      Serial.println("Sensor Initialization Successful\n\r");
    }
      else{
        Serial.println("failed RGB");
    }
    ///////////////////////////////////////////////////////
#endif
    
}


// learn about SD card and file read/write at https://www.arduino.cc/en/Reference/SD


uint8_t i=0;
int loopCnt = 0;
void loop()
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Read sensor values (16 bit integers)
  unsigned int red = RGB_sensor.readRed();
  unsigned int green = RGB_sensor.readGreen();
  unsigned int blue = RGB_sensor.readBlue();

  //code for printing out RGB sensor readings
  // Print out readings, change HEX to DEC if you prefer decimal output
  //Serial.print("Red: "); Serial.println(red,DEC);
  //Serial.print("Green: "); Serial.println(green,DEC);
  //Serial.print("Blue: "); Serial.println(blue,DEC);
  //Serial.println();
  //delay(100);
  //code for triggering cam when red light is shined on RGB
  /*if (red > green){
    Serial.println("Takin picture cap'n");
    digitalWrite(D9_IO_PWR, HIGH);
    delay(100);
    digitalWrite(D9_IO_PWR, LOW);
  }
  else{
    Serial.println("Too much blue and green");
  }*/
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
    //Serial.println("get REEEEADDDDDY");
    //Serial.print(F("x-axis good good: acceleration : "));
    //Serial.print(myIMU0.accelCount[0],1);
  
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
 
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //Serial.print(loopCnt++, HEX); Serial.print("  "); Serial.println("chip select doink test");
  digitalWrite(LED1, HIGH);
  //delay(100);
  //digitalWrite(D9_IO_PWR, HIGH);*********************
  digitalWrite(D4_IO_REC, HIGH);    // This pin might have an issue with uSD
  for(int indx = 0; indx< 5; indx++)
  {
// GABE: look for activity on D4_IO_REC during this loop and the one below - if there is
// activity, it is due to the logfile.print using the sd card default chip select (which is D4)
    
    logfile.print("A0 = "); logfile.println(analogRead(0));
    delay(100);
  }

  digitalWrite(LED1, LOW);  
  //digitalWrite(D9_IO_PWR, LOW);******************************
  digitalWrite(D4_IO_REC, LOW);    // This pin might have an issue with uSD
  for(int indx = 0; indx< 5; indx++)
  {
    logfile.print("A0 = "); logfile.println(analogRead(0));
    //Serial.print(analogRead(0));
    //delay(100);
    //Serial.println("Hello There");
    delay(100);
  }


  if(Serial2.available())     // if rx and tx are 'looped back (shorted), then what is sent on TX should appear on RX
  {
    Serial.print(" -> 0x");Serial.print(Serial2.read(),HEX);
  }

  if(Serial.available())     // if rx and tx are 'looped back (shorted), then what is sent on TX should appear on RX
  {
    uint8_t inChar = Serial.read();
    switch(inChar)
    {
      case 'a':
      case 'A':
        Serial.println("aye");
        digitalWrite(D9_IO_PWR, HIGH);
        delay(100);
        digitalWrite(D9_IO_PWR, LOW);
        loopCnt = 0;
        break;
      default:
        Serial.write(inChar);
        break;
    }
  }  

#ifdef NOCODE  
  digitalWrite(8, HIGH);
  logfile.print("A0 = "); logfile.println(analogRead(0));
  //Serial.print("A0 = "); Serial.println(analogRead(0));
  digitalWrite(8, LOW);

  Serial2.write(0x55);
  if(Serial2.available())     // if rx and tx are 'looped back (shorted), then what is sent on TX should appear on RX
  {
    Serial.print(" -> 0x");Serial.print(Serial2.read(),HEX);
  }
  Serial.println();
  delay(100);
#endif
  
}




//blink out an error code
void error(uint8_t errno)
{
  while(1)
  {
    uint8_t i;
    for (i=0; i<errno; i++)
    {
      digitalWrite(LEDRED, HIGH);
      delay(100);
      digitalWrite(LEDRED, LOW);
      delay(100);
      Serial.println("\r\nHello problems");
    }
    for (i=errno; i<10; i++)
    {
      delay(200);
    }
  }
}



void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}
