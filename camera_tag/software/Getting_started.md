# Getting started with the board

### Materials: Computer, microcontroller (main board), USB to micro-USB cable.
### Objective: Communicate with your microcontroller and have it respond to test foor succesfull communication.

#### Step 1: How to get set up (downloading the necessary drivers)
https://learn.adafruit.com/adafruit-feather-m0-adalogger/setup
There’s two important pages in the Adafruit instructions ‘Arduino IDE setup’ and ‘Using with Arduino IDE’. Follow the first ‘Arduino IDE setup’, download the arduino software. Then, for the specific drivers go to the next page of the guide 'Using with Arduino IDE.' There is also an arrow at the end of the page that will take you to it. Make sure to go through the setup **step by step** to the end of each page.  

```
Note: The main board uses the same processor as the Adafruit Feather M0 Adalogger. Therefore, the driver for the M0 is required to communicate with the board. The instructions above will guide you through the process.
```

#### Step 2: Test communication through LED
1. Open the Arduino IDE
2. Select **Tools** > **Board** > **Adafruit Feather M0**
3. Make sure the **Tools** > **Port** reads > **Adafruit Feather M0**
   1. If this is not the case (either the computer isn't registering the device, there are multiple instances, etc.) go to the Restarting your board file.
4. 
