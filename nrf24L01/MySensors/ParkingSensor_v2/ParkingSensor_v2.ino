/**
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Ole Anders Bøe
 * 
 * DESCRIPTION
 * Parking sensor using a neopixel led ring and distance sensor (HC-SR04).
 * Configure the digital pins used for distance sensor and neopixels below.
 */

#include <FastLED.h>

#define LED_PIN   6 // NeoPixels input pin

#define trigPin  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define echoPin  11  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define NUMPIXELS      16 // Number of nexpixels in ring/strip
#define MAX_INTESITY   60  // Intesity of leds (in percentage). Remeber more intesity requires more power.

// The maximum rated measuring range for the HC-SR04 is about 400-500cm.
#define MAX_DISTANCE 180 // Max distance we want to start indicating green (in cm)
#define PANIC_DISTANCE 90  // Mix distance we red warning indication should be active (in cm)

#define PARK_OFF_TIMEOUT 15000 // Number of milliseconds until turning off light when parked.

// Enable debug prints to serial monitor

CRGB leds[NUMPIXELS];

unsigned long sendInterval = 1000;  // Send park status at maximum every 5 second.
unsigned long lastSend;

unsigned long blinkInterval = 100; // blink interval (milliseconds)
unsigned long lastBlinkPeriod;
bool blinkColor = true;

// To make a fading motion on the led ring/tape we only move one pixel/distDebounce time
unsigned long distDebounce = 30; 
unsigned long lastDebouncePeriod;
int numLightPixels=0;
int skipZero=0;
int prevDist=0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting distance sensor");
  delay( 2000 ); // power-up safety delay
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUMPIXELS);   
  FastLED.setBrightness(  MAX_INTESITY );
}

void loop() {
  FastLED.delay(100);
  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration;
 
  // The sensor is triggered by a HIGH pulse of 
  // 10 or more microseconds.
  // Give a short LOW pulse beforehand to 
  // ensure a clean HIGH pulse:
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
 
  // convert the time into a distance
  int fullDist = microsecondsToCentimeters(duration);
  Serial.print("fullDist : ");
  Serial.println(fullDist);

  unsigned long now = millis(); 
  int deltaDist = prevDist - fullDist;
  if(abs(deltaDist) < 3)
  { 
    // Ingen endring
    fullDist = prevDist; 
  }
  else
  { 
    prevDist = fullDist; 
    // Endret 
    lastSend = now;
  }
  
  Serial.print(fullDist);  Serial.println("cm"); 

  int displayDist = min(fullDist, MAX_DISTANCE);

  // Check if it is time to alter the leds
  if (now-lastDebouncePeriod > distDebounce) {
    lastDebouncePeriod = now;

    if ( now-lastSend > PARK_OFF_TIMEOUT) {
      // We've been parked for a while now. Turn off all pixels
      for(int i=0;i<NUMPIXELS;i++){
        leds[i] = CRGB( 0, 0, 0);        
        FastLED.delay(100);
      }
      //FastLED.clear();  // All off
    } else {
      if (displayDist == 0) {
        // No reading from sensor, assume no object found
        numLightPixels--;
        FastLED.delay(100);
      } else {
        int newLightPixels = NUMPIXELS - (NUMPIXELS*(displayDist-PANIC_DISTANCE)/(MAX_DISTANCE-PANIC_DISTANCE));   
        
         Serial.print("newLightPx / numLightPx : "); Serial.print(newLightPixels);Serial.print("/");Serial.println(numLightPixels);
        if (newLightPixels>numLightPixels) {
          // Fast raise
          numLightPixels += max((newLightPixels - numLightPixels) / 2, 1);
        } else if (newLightPixels<numLightPixels) {
          // Slow decent
          numLightPixels--;
        }
      }
  
      if (numLightPixels>=NUMPIXELS) {
// Do some intense red blinking 
        if (now-lastBlinkPeriod > blinkInterval) {
          blinkColor = !blinkColor;
          lastBlinkPeriod = now;
        }
        for(int i=0;i<numLightPixels;i++){
          leds[i] = CRGB( blinkColor?255*MAX_INTESITY/100:0,0,0);
        }              
      } else {
       // numLightPixels = min(numLightPixels, NUMPIXELS);
        //FastLED.clear();  
        for(int i=0;i<numLightPixels;i++){
          int r = 255 * i/NUMPIXELS;
          int g = 255 - r;     
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          leds[i] = CRGB( r*MAX_INTESITY/100,g*MAX_INTESITY/100,0);
        }        
        // Turn off the rest
        //for(int i=numLightPixels;i<NUMPIXELS;i++){
        //  leds[i] = CRGB( 0, 0, 0);
        //}
      }
    }
  }
  Serial.println(numLightPixels);  
  FastLED.show(); // This sends the updated pixel color to the hardware.

}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per
  // centimeter. The ping travels out and back, so to find 
  //the distance of the object we take half of the distance 
  //travelled.
  return microseconds / 29 / 2;
}
