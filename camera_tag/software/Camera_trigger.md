# Camera Trigger
### 1. What is it/how does it work
### 2. How to use it

![Cam Trigger](https://user-images.githubusercontent.com/52707386/63304094-b63f5e80-c296-11e9-8e68-4f7866a1bdb1.jpg)

| Symbol        | Meaning       | Name on Code  | Pin       | What is it?       |
| ------------- |:-------------:| -------------:|-------------:|----------------:  |
| R             | Record        | D4_IO_PWR     | 4  |Transistor drain  |
| G             | Ground        | N/A           | N/A |Transistor source  |
| P             | Power         | D9_IO_REC     | 9  |Transistor drain  |

### 1. What is the trigger?  
   The triggers (there are two) are two logic level field effect transistors (FETs) that share the same source (ground). A transistor acts like a switch that you flip with a command instead of with your finger. This allows you to 'flip' on/off any device connected to the trigger. *Logic level* is in reference to the amount of power required to flip the switch. A logic level transistor can be 'flipped' with less voltage, such as what a microcontroller like this board can provide. Instructions on how to prepare a device for this sort of triggering are found in **camera_tag/electrical/camera_prep.**  
   
### 2. How to use the onboard triggers 
   To trigger the transistor you need to output power to either pin 4 or 9 depending on which you want to use (they both work). In some of the example code provided these pins are defined as either D4-IO_PWR or D9_IO_REC. There is example code provided in **camera_tag/software/trigg_code.** 
