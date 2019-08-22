Common issues and how to resolve them
==========================================

## 1. Board not recognized on IDE
   1. Restart
      With the board connected to your computer. Run a simple program on the IDE such as **Blink_mod,** then click on upload (the arrow on the top left). Then, while the load procedure is goig through **compiling sketch** touch a wire from **Reset** to **Ground** twice in quick succession.
      ![edit2](https://user-images.githubusercontent.com/52707386/63554840-a1abd200-c4f3-11e9-9721-d27e66fb61fc.jpg)
![20190822_153921](https://user-images.githubusercontent.com/52707386/63554845-a4a6c280-c4f3-11e9-9f2f-c9955bdaa593.jpg) 

## 2. Code not uploading 
   1. Board selected
      Make sure the board selected is the **Adafruit Feather M0**
 ![image](https://user-images.githubusercontent.com/52707386/63554976-17b03900-c4f4-11e9-974f-7607ca5a4909.png)
 
   2. Port
      Make sure the port with the board is selected   
   
   ![image](https://user-images.githubusercontent.com/52707386/63555094-7aa1d000-c4f4-11e9-97f7-20fb0d5a561c.png)
   
## 3. Multiple instances appear of board on same port
   1. Save your file, close all instances of the Arduino IDE and restart the program.
   
## 4. Upload process stuck
   1. board selection
         Make sure the correct board is selected (refer to image on 'Port')
   2. code
         Your code might have an error, try a sketch you know runs to test if this is actually the problem.
   3. Restart
         Close all IDE instances and restart.
