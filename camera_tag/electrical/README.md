# Shark Cafe Cam Main Board Information
## Files
1. Schematic
2. Board design
## Internal components

1. Processor
2. Sensors  
3. Camera trigger
```
Note on camera trigger transistors: they allow for switching the camera on/off and record/stop. These transistors work by allowing current to flow through the camera when they are triggered. In a way they act as an electrical switch, when power is sent to them you're flipping the switch 'on', thus activating the camera, when no power is sent, the switch is flipped 'off' and the camera disactivates.
```

### Processor

| Part         | Function          | Board Code    | Description       | Link and part number      |
| --------------------|:-------------:|:-------------:|-----------------|----------------- |
| ![image](https://user-images.githubusercontent.com/52707386/63448839-b143f180-c3f3-11e9-8a64-d5150a3e8d03.png) | Processor | N/A | Central processing. Same processor found in Feather M0, can be accessed as such through Arduino IDE.| ATSAMD21G18A |

### Sensors

| Part breakout board         | Function          | Board Code    | Description       | Link and part number      |
| --------------------|:-------------:|:-------------:|-----------------|:---------------------------:|
| ![image](https://user-images.githubusercontent.com/52707386/63448032-f23b0680-c3f1-11e9-9f2b-210ce0e2d542.png)  | RGB sensor  | U1  | Red, green, blue light sensor  |  [ISL29125](https://www.sparkfun.com/products/12829)   |
| ![image](https://user-images.githubusercontent.com/52707386/63448396-ab014580-c3f2-11e9-9538-76c5c1f9470b.png)  | IMU        | U4           | Accelerometer, gyroscope, and compass | [MPU-9250](https://www.invensense.com/products/motion-tracking/9-axis/mpu-9250/)   |
| ![image](https://user-images.githubusercontent.com/52707386/63448686-4c889700-c3f3-11e9-99aa-de4e05dbfba7.png)  | Capacitive sensing         |   U6   | 4 Channel Capacitance to Digital Converter  | [FDC1004](http://www.ti.com/product/FDC1004)  |

### Trigger (for camera or other device. Transistor.)

| Location on board         | Function          | Board Code    | Description       | Link and part number      |
| --------------------|:-------------:| -------------|-----------------|---------------------------|
| ![Cam Trigger](https://user-images.githubusercontent.com/52707386/63304094-b63f5e80-c296-11e9-8e68-4f7866a1bdb1.jpg)  |  2 FETs, one turns the camera on/off and the other records/stops.  | CAM1  | Trigger for camera. R and P are transistor drains, G is the common source.  |  [Transistors](https://learn.sparkfun.com/tutorials/transistors/all#extending-the-water-analogy)   |

### Electrical components

| Part Number         | Function          | Board Code    | Description       | Link       |
|:--------------------:|:-------------:|:-------------:|:-----------------:|:-----------------:|
| SN74LVC2G66DCUR  | Switch         | U5    | Dual Bilateral Analog Switch  | [SN74LVC2G66DCUR](http://www.ti.com/lit/ds/symlink/sn74lvc2g66.pdf)  |
| TPS22918            | Load Switch         | U8     | Details in datasheet  | [TPS22918](http://www.ti.com/product/TPS22918)  |
| 2N7002BKS           | MOSFET         | D9_IO_REC     | Transistor drain  | [2N7002BKS](https://assets.nexperia.com/documents/data-sheet/2N7002BKS.pdf)  |
| ZXCT1109            | Current sense monitor         |   U9   | Details in datasheet  | [ZXCT1109](https://www.diodes.com/assets/Datasheets/ZXCT1107_10.pdf)  |
| TPS78033            | LDO regulator         | U7     | Details in datasheet  | [TPS78033](https://www.findchips.com/detail/tps78033/2477-Texas%20Instruments?quantity=1)  |
| MCP73831            | Charge Management controller         |  U3    | Details in datasheet  | [MCP73831](https://cdn.sparkfun.com/assets/learn_tutorials/6/9/5/MCP738312.pdf)  |
| Direct wire uSD connect        | Micro SD card connector         | Pins for soldering MicroSD card on.     | J9  | N/A  |
| USB-MICROB 5v                | Data transfer and power with computer         | J6     | Micro USB connector port  | N/A  |
   
 ### *For information on how to communicate with the board and each sensor, check out the software section.*
