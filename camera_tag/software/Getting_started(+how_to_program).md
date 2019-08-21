# Getting started with the board

*The word 'communicate' is used in this guide as: exchange of information between you and your board. Succesful communication allows for full manipulation of the board and its components, whether gathering information, switching on/off, etc.*

### Materials: Computer, microcontroller (main board), USB to micro-USB cable.
### Objective: Communicate with your microcontroller and have it respond to test for succesful communication.

 *In an effort to get straight to programing the board we will offer a condensed version of getting started with the Arduino integrated development environment (IDE). The Arduino IDE is a platform created by Arduino to interface with a variety of microcontrollers.*

#### Step 1: Download the Arduino IDE
Go to [Arduino Main Page](https://www.arduino.cc/en/main/software) and scroll down to:
![image](https://user-images.githubusercontent.com/52707386/63463747-ae0c2e00-c412-11e9-9406-0761ba84e443.png)
Select the appropriate download for your system and follow the instructions.

#### Step 2: How to get set up (downloading the necessary drivers)
https://learn.adafruit.com/adafruit-feather-m0-adalogger/setup
There’s two important pages in the Adafruit instructions ‘Arduino IDE setup’ and ‘Using with Arduino IDE’. Follow the first ‘Arduino IDE setup’, download the arduino software. Then, for the specific drivers go to the next page of the guide 'Using with Arduino IDE.' There is also an arrow at the end of the page that will take you to it. Make sure to go through the setup **step by step** to the end of each page.    

**Note: The main board uses the same processor as the Adafruit Feather M0 Adalogger. Therefore, the driver for the M0 is required to communicate with the board. The instructions above will guide you through the process.**

#### Step 3: Test communication through an onboard LED (making use of example code)
1. Open the Arduino IDE.
2. Connect the board (micro USB) to your computer (USB).
3. On the Arduino IDE select **Tools** > **Board** > **Adafruit Feather M0**
4. Make sure the **Tools** > **Port** reads '**Adafruit Feather M0**'
   1. If this is not the case (either the computer isn't registering the device, there are multiple instances, etc.) go to the Restarting your board file.
5. Select **File** > **Examples** > **Basics** > **Blink**
   1. Once the 'Blink' file opens, switch any 'LED_BUILTIN' statement for a '26.' Alternatively, open the *mod_Blink.ino* file availble in this repository within the *mod_Blink* folder.
      1. If LED 1 begins to blink, then you have been succesful.
   ```
   Note: 26 is the pin designation for Red LED 1 on the board.
   ```
6. Now that you have succesfully communicated with the board, navigate to specific sensors or the general test code to communicate with other components on the board.

#### Resources:
##### In the Arduino IDE 
- **File** > **Examples** for pre-built code that you can use
- **Help** > **Reference** for function names and descriptions
- **Help** > **Troubleshooting**
##### General
 - [Arduino](https://www.arduino.cc/en/Guide/ArduinoUno)
 - [Adafruit lessons](https://learn.adafruit.com/series/learn-arduino)
 - [SparkFun](https://learn.sparkfun.com/tutorials)
