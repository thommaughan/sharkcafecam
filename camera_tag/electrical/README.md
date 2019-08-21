# Shark Cafe Cam Main Board Information
## Index
1. Schematic
2. Board design
#### Internal
1. Processor
2. Sensors  

| Part Number         | Function          | Board Code    | Description       | Link       |
| --------------------|:-------------:| -------------|-----------------|----------------- |
| ISL29125            | RGB sensor    | U1     | Red, green, blue light sensor  | https://www.sparkfun.com/products/12829 |
| MPU-9250            | Ground        | N/A           | Transistor Source | https://www.invensense.com/products/motion-tracking/9-axis/mpu-9250/   |
| FDC1004             | Capacitive sensing         | D9_IO_REC     | Transistor drain  | Transistor drain  |
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
