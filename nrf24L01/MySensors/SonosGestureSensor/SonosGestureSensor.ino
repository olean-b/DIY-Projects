/****************************************************************
  Title: MySensors enabled, gesture controlled Sonos 
   
  Supported gesture:
  1. Left to right stroke: turns music on
  2. Right to left stroke: turns music off
  3. Down to up stroke: volume up
  4. Up to down stroke: volume down
  5. Near : Next track
  6. Far  : Prev. track
  
  The gesture sensor ues in this Sketch is an APDS-9960 RGB and Gesture Sensor, sold by SparkFun. They can be found on eBay and Aliexpress as well.
  See Sparkfuns hookup guide for pin lay-out ( https://learn.sparkfun.com/tutorials/apds-9960-rgb-and-gesture-sensor-hookup-guide ) 
  IMPORTANT: The APDS-9960 can only accept 3.3V! Use bi direction level converter when using another Arduino than a Pro Mini 3.3V

  
  Revision history:
  19-03-2017 Version 1.0
****************************************************************/
// #define MY_DEBUG
#define MY_NODE_ID 13
#define MY_REPEATER_FEATURE
#define MY_RADIO_NRF24


// Import libraries used by the Sketch.
#include <SPI.h>
#include <MySensors.h>  
#include <Wire.h>
#include <SparkFun_APDS9960.h>


#define APDS9960_INT  2   // Needs to be an interrupt pin. We should be able to use the Arduino's pin 3 as well.
#define CHILD_ID      1   // Id of the sensor child

SparkFun_APDS9960 apds = SparkFun_APDS9960(); // Initialize a SparkFun_APDS9960 object. This does all the magic incl. i2c communication with the sensor.

int isr_flag = 0;                             // interrupt flag, triggered when a gesture has been dectected. Used to detect gesture detection in the interrupt handler
                                              // the actual handling is done in the main loop. This allows is the keep the interrupt handler is lightweight is possible
                                              // which is a good practice. Otherwise you'll get some behaviour you don't expect.
                                              
MyMessage ctrlMsg(CHILD_ID,V_VAR1);
unsigned long timer = 0;

void setup()  
{  
  pinMode(APDS9960_INT, INPUT);   // Set interrupt pin as input. @@Note: this should be handled my the library. But it isn't
    // Initialize interrupt service routine
  attachInterrupt(digitalPinToInterrupt(APDS9960_INT), interruptRoutine, FALLING);
  
  // Initialize APDS-9960 (configure I2C and initial values)
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
    apds.setGestureGain( 1 );
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  
  // Start running the APDS-9960 gesture sensor engine
  if ( apds.enableGestureSensor(true) ) {
    Serial.println(F("Gesture sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during gesture sensor init!"));
  }
  
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Gesture controlled Sonos", "1.0");
}

// interrupt handler. Is being triggered by the gesture sensor whenever a gesture has been detected. We've setup this up in the setup.
void interruptRoutine() {
  isr_flag = 1;
}

void loop()     
{ 
  if( isr_flag == 1 ) {
    detachInterrupt(digitalPinToInterrupt(APDS9960_INT));
    handleGesture();
    isr_flag = 0;
    attachInterrupt(digitalPinToInterrupt(APDS9960_INT), interruptRoutine, FALLING);
  }
}

// Determine gesture and report to gw.
void handleGesture() {
  if ( apds.isGestureAvailable() ) { // Check if there's a gesture available. Which should be, because the interrupt handler told us so.
    send(ctrlMsg.set(apds.readGesture()));  // Send GestureType (int 0 - 7)
    /* Direction definitions 
        enum { DIR_NONE,
               DIR_LEFT,
               DIR_RIGHT,
               DIR_UP,
               DIR_DOWN,
               DIR_NEAR,
               DIR_FAR,
               DIR_ALL
             };  
    */
  }
}

