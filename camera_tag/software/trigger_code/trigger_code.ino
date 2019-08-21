*/
This sample code will turn on a camera, wait, record, wait, then turn off. This will be repeated indefinately.
This code snippet is to show the pin designation and command necessary to access these transistors.
You may incorporate this code into logical statements to turn a device on when a certain condition is met.

Code by:
Gabriel M. Santos E. and Thom Maughan

Adapted from 'Blink' from the Arduino examples
*/


#define D9_IO_PWR       9       //9  //PA7  { PORTA,  7, PIO_ANALOG, (PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel7, PWM1_CH1, TCC1_CH1, EXTERNAL_INT_7 }, // p12
#define D4_IO_REC       4       //4 //PA8  { PORTA,  8, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), ADC_Channel16, PWM0_CH0, TCC0_CH0, EXTERNAL_INT_NMI }, // p23


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin D9_IO_PWR as an output.
  pinMode(D9_IO_PWR, OUTPUT);
  // initialize digital pin D4_IO_REC as an output.
  pinMode(D4_IO_REC, OUTPUT);
}

// the loop function runs over and over again forever
// potential scenario where the camera being used has two seperate buttons that you need to trigger to take video or pictures
void loop() {
  digitalWrite(D9_IO_PWR, HIGH);    // press the camera 'on' button to turn the camera on
  delay(100);                       // wait for a 1/10th second
  digitalWrite(D9_IO_PWR, LOW);     // stop pressing the button (if you were to hold the button down some cameras would turn on then off)
  delay(2000);                      // wait for the camera to boot up
  digitalWrite(D4_IO_REC, HIGH;     // press the record button
  delay(100);                       // wait for a 1/10th second
  digitalWrite(D4_IO_REC, HIGH;     // release the button
  delay(10000);                     // record for 10 seconds
  digitalWrite(D4_IO_REC, HIGH;     // press the record button to stop recording
  delay(100);                       // wait for a 1/10th second
  digitalWrite(D4_IO_REC, HIGH;     // release the button
  digitalWrite(D9_IO_PWR, HIGH);    // press the camera 'on' button to turn the camera off
  delay(2000);                      // wait for two seconds (in some cameras pressing the power button for a longer time, turns the camera off)
  digitalWrite(D9_IO_PWR, LOW);     // stop pressing the button
  delay(2000);                      // wait two second for loop to restart
  }
