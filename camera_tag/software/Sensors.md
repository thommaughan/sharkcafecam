# How to access sensors on the board (DIY demo style instructions)  
In an effort to teach you how to access any sensor on the board, we will offer instructions and then a challenge for a simple sensor and a more complex one. The purpose of this is to get you comfortable with navigating the code necessary to utilize these sensors in any way you please. If you already feel comfortable navigating code, feel free to download the sample code directly (camera_tag/software/sample_code).  

Sensor Libraries  
To facilitate communication with a sensor you can download a library.
```
Note: A library is a compilation of functions for a specific module or sensor. It allows for facilitated control of a device by offering the code to control it in more easily understood snippets. You can tell a library is being used if you see: #include <libraryname.h> at the top of the program. To figure out what functions are available in the library you can look up the library name online or look through its folder you downloaded.
```
1. Download the SFE_ISL29125 folder from */Libraries.*
2. Move the folder into your computer **Documents** > **Arduino** > **libraries**. Make sure both the .h and .cpp files are in the folder you're adding.
3. Restart your Arduino IDE.
4. On the IDE select **File** > **Examples** > **SFE_ISL29125** > **ISL29125_basics**
5. Run the code
   1. Open the serial monitor (ctrl+shift+m) to view the RGB sensor readings.  
   Note: if you want the readings as decimal values change 'HEX' to 'DEC' in the code.
