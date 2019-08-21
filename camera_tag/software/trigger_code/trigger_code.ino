/*
This sample code will turn on a camera, wait, then turn off. This will be repeated indefinately.
This code snippet is to show the pin designation and command necessary to access these transistors.
You may incorporate this code into logical statements to turn a device on when a certain condition is met.
Code by:
Gabriel M. Santos E. and Thom Maughan
Adapted from 'Blink' from the Arduino examples
*/

#define D9_IO_PWR       9       //9  //PA7  { PORTA,  7, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel7, PWM1_CH1, TCC1_CH1, EXTERNAL_INT_7 }, // p12


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin D9_IO_PWR as an output.
  pinMode(D9_IO_PWR, OUTPUT);
  // set the pin to LOW (no power)
  digitalWrite(D9_IO_PWR, LOW);
}

// the loop function runs over and over again forever
// code that turns the camera on, waits, then turns it off. This same code can be applied to the other trigger pin.
void loop() {
  delay(5000);                      // wait 5 seconds
  digitalWrite(D9_IO_PWR, HIGH);    // press the camera 'power' button to turn the camera on
  delay(100);                       // wait for a 1/10th second
  digitalWrite(D9_IO_PWR, LOW);     // stop pressing the button (if you were to hold the button down some cameras would turn on then off)
  delay(6000);                      // wait for the camera to boot up (6 seconds in this case)
  digitalWrite(D9_IO_PWR, HIGH);    // press the camera 'power' button to turn the camera off 
  delay(5000);                      // wait for 5 seconds (in some cameras pressing the power button for a longer time, turns the camera off)
  digitalWrite(D9_IO_PWR, LOW);     // stop pressing the button
  delay(10000);                     // wait ten seconds
  }
