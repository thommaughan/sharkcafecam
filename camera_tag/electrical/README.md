# Shark Cafe Cam Main Board Information
## Index
1. Schematic
2. Board design
#### Internal
1. Processor
2. Sensors  

### Sensors

| Part breakout board         | Function          | Board Code    | Description       | Link and part number      |
| --------------------|:-------------:| -------------|-----------------|---------------------------|
| ![image](https://user-images.githubusercontent.com/52707386/63448032-f23b0680-c3f1-11e9-9f2b-210ce0e2d542.png)  | RGB sensor  | U1  | Red, green, blue light sensor  |  [ISL29125](https://www.sparkfun.com/products/12829)   |
| ![image](https://user-images.githubusercontent.com/52707386/63448396-ab014580-c3f2-11e9-9538-76c5c1f9470b.png)  | IMU        | U4           | Accelerometer, gyroscope, and compass | [MPU-9250](https://www.invensense.com/products/motion-tracking/9-axis/mpu-9250/)   |
| ![image](https://user-images.githubusercontent.com/52707386/63448686-4c889700-c3f3-11e9-99aa-de4e05dbfba7.png)  | Capacitive sensing         |   U6   | 4 Channel Capacitance to Digital Converter  | [FDC1004](http://www.ti.com/product/FDC1004)  |

### Electrical components

| Part Number         | Function          | Board Code    | Description       | Link       |
| --------------------|:-------------:| -------------|-----------------|----------------- |
| SN74LVC2G66DCUR            | Dual Bilateral Analog Switch         | XXXX    | Transistor drain  | http://www.ti.com/lit/ds/symlink/sn74lvc2g66.pdf  |
| isBUS               | Power         | D9_IO_REC     | Transistor drain  | Transistor drain  |
| TPS22918            | Load Switch         | XXXXX     | Transistor drain  | Transistor drain  |
| 2N7002BKS           | MOSFET         | D9_IO_REC     | Transistor drain  | https://assets.nexperia.com/documents/data-sheet/2N7002BKS.pdf  |
| ZXCT1109            | Power         | current sense monitor     | Transistor drain  | https://www.diodes.com/assets/Datasheets/ZXCT1107_10.pdf  |
| TPS78033            | LDO regulator         | D9_IO_REC     | Transistor drain  | https://www.alldatasheet.com/view.jsp?Searchword=TPS78033&sField=2  |
| MCP73831            | Charge Management controller         | D9_IO_REC     | Transistor drain  | https://cdn.sparkfun.com/assets/learn_tutorials/6/9/5/MCP738312.pdf  |
| SD connector        | Power         | D9_IO_REC     | Transistor drain  |
| Micro USB                | Power         | D9_IO_REC     | Transistor drain  |
| 2N7002BKS             | Power         | D9_IO_REC     | Transistor drain  |

   
 For information on how to communicate with the board and each sensor, check out the software section
