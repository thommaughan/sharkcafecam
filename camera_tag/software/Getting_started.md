# Getting started with the board

*The word 'communicate' is used in this guide as: exchange of information between you and your board. Succesfull communication allows for full manipulation of the board and its components, whether gathering information, switching on/off, etc.*

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
2. Connect the board (micro USB) to your computer (USB)
3. Connect a battery to the board----------------------MORE ON THIS
![20190819_105314](https://user-images.githubusercontent.com/52707386/63287986-76658080-c270-11e9-868f-2fd6761db1a0.jpg)
*Make sure the positive and negative terminals of your battery are connected to the white connecter in line with how the board's positive and negative terminal are aligned. We had to switch the wires on the battery. To do this, follow this simple guide::::::::::::*
![20190819_105235](https://user-images.githubusercontent.com/52707386/63288025-8aa97d80-c270-11e9-940b-5117c6865c81.jpg)
*The brown cable in this setup is ground*
![20190819_110233](https://user-images.githubusercontent.com/52707386/63288356-54203280-c271-11e9-83fe-4e64694c0720.jpg)
![20190819_110309](https://user-images.githubusercontent.com/52707386/63288360-55e9f600-c271-11e9-954d-1024d07e2489.jpg)
4. Select **Tools** > **Board** > **Adafruit Feather M0**
5. Make sure the **Tools** > **Port** reads > **Adafruit Feather M0**
   1. If this is not the case (either the computer isn't registering the device, there are multiple instances, etc.) go to the Restarting your board file.
6. Select **File** > **Examples** > **Basics** > **Blink**
   1. Once the 'Blink' file opens, switch any 'LED_BUILTIN' statement for a '26.'
      1. If LED 1 begins to blink, then you have been succesfull.
   ```
   Note: 26 is the pin designation for Red LED 1 on the board.
   ```
7. Now that you have succesfully communicated with the board, navigate to specific sensors or the general test code to communicate with other components on the board.
