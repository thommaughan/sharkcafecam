# Access sensors on the board
In an effort to teach you how to access *any* sensor on the board, we will offer detailed instructions for a simple sensor and a more complex one. For the rest, the part number and library will be provided. The idea is that you'll follow the same procedure with the rest of the sensors as you did with the two provided. The purpose of this is to get you comfortable with navigating the code necessary to utilize these sensors in any way you please. If you already feel comfortable navigating code, feel free to download the sample code directly (camera_tag/software/sample_code).  

Sensor Libraries  
To facilitate communication with a sensor you can download a library.
```
Note: A library is a compilation of functions for a specific module or sensor. It allows for facilitated control of a device by offering the code to control it in more easily understood snippets. You can tell a library is being used if you see: #include <libraryname.h> at the top of the program. To figure out what functions are available in the library you can look up the library name online or look through its folder you downloaded.
```  
### Red Green Blue (RGB) sensor (ISL29125)
![RGB](https://user-images.githubusercontent.com/52707386/63302093-1c28e780-c291-11e9-88fd-69a57f3260c7.jpg)
1. Download the **SFE_ISL29125** folder from */Libraries.*
2. Move the folder into your computer **Documents** > **Arduino** > **libraries**. Make sure both the .h and .cpp files are in the folder you're adding.
3. Restart your Arduino IDE.
4. On the IDE select **File** > **Examples** > **SFE_ISL29125** > **ISL29125_basics**
5. Run the code
   1. Open the serial monitor (ctrl+shift+m) to view the RGB sensor readings.  
   Note: if you want the readings as decimal values change 'HEX' to 'DEC' in the code.
   
### Inertial Measurement Unit (IMU) (MPU-9250)
![MPU](https://user-images.githubusercontent.com/52707386/63302397-d4ef2680-c291-11e9-90c9-8af89acb0f88.jpg)
1. Download the **SparkFun_MPU-9250** folder from */Libraries.*
2. Move the folder into your computer **Documents** > **Arduino** > **libraries**. Make sure both the .h and .cpp files are in the folder you're adding. Check the src folder within.
3. Restart your Arduino IDE.
4. Download the **IMU_code** sketch from */software.*
5. Run **IMU_code** to receive IMU measurement values.
   1. Open the serial monitor (ctrl+shift+m) to view the sensor readings.
