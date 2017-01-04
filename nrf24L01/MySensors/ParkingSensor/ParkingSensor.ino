/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Henrik Ekblad
 * 
 * DESCRIPTION
 * Parking sensor using a neopixel led ring and distance sensor (HC-SR04).
 * Configure the digital pins used for distance sensor and neopixels below.
 * NOTE! Remeber to feed leds and distance sensor serparatly from your Arduino. 
 * It will probably not survive feeding more than a couple of LEDs. You 
 * can also adjust intesity below to reduce the power requirements.
 * 
 * Sends parking status to the controller as a DOOR sensor if SEND_STATUS_TO_CONTROLLER 
 * is defined below. You can also use this _standalone_ without any radio by 
 * removing the c define.
 */

#include <FastLED.h>

#define LED_PIN   6 // NeoPixels input pin

#define trigPin  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define echoPin  11  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define NUMPIXELS      16 // Number of nexpixels in ring/strip
#define MAX_INTESITY   80  // Intesity of leds (in percentage). Remeber more intesity requires more power.

// The maximum rated measuring range for the HC-SR04 is about 400-500cm.
#define MAX_DISTANCE 180 // Max distance we want to start indicating green (in cm)
#define PANIC_DISTANCE 40 // Mix distance we red warning indication should be active (in cm)
#define PARKED_DISTANCE 70 // Distance when "parked signal" should be sent to controller (in cm)

#define PARK_OFF_TIMEOUT 20000 // Number of milliseconds until turning off light when parked.

// Enable debug prints to serial monitor

CRGB leds[NUMPIXELS];

unsigned long sendInterval = 5000;  // Send park status at maximum every 5 second.
unsigned long lastSend;

int oldParkedStatus=-1;

unsigned long blinkInterval = 100; // blink interval (milliseconds)
unsigned long lastBlinkPeriod;
bool blinkColor = true;

// To make a fading motion on the led ring/tape we only move one pixel/distDebounce time
unsigned long distDebounce = 30; 
unsigned long lastDebouncePeriod;
int numLightPixels=0;
int skipZero=0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting distance sensor");
  delay( 2000 ); // power-up safety delay
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUMPIXELS);   
  
  FastLED.setBrightness(  MAX_INTESITY );

  Serial.println("FastLED initialized");
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
 
 // Serial.print(fullDist);  Serial.println("cm"); 
  unsigned long now = millis(); 

  int displayDist = min(fullDist, MAX_DISTANCE);

  if (displayDist == 0 && skipZero<10) {
    // Try to filter zero readings
    skipZero++;
    Serial.println("SkipZero");
    return;
  }
  
  // Check if it is time to alter the leds
  if (now-lastDebouncePeriod > distDebounce) {
    lastDebouncePeriod = now;

    // Update parked status
    int parked = displayDist != 0 && displayDist<PARKED_DISTANCE;
    if (parked != oldParkedStatus && now-lastSend > sendInterval) {
      if (parked)
        Serial.println("Car Parked");
      else
        Serial.println("Car Gone");

      // send(msg.set(parked)); 

      oldParkedStatus = parked;
      lastSend = now;
    }

    if (parked && now-lastSend > PARK_OFF_TIMEOUT) {
      // We've been parked for a while now. Turn off all pixels
      for(int i=0;i<NUMPIXELS;i++){
        leds[i] = CRGB( 0, 0, 0);
        // Serial.println("Turn off all pixels");
        FastLED.delay(200);
      }
    } else {
      if (displayDist == 0) {
        // No reading from sensor, assume no object found
        numLightPixels--;
        FastLED.delay(100);
      } else {
        skipZero = 0;
        int newLightPixels = NUMPIXELS - (NUMPIXELS*(displayDist-PANIC_DISTANCE)/(MAX_DISTANCE-PANIC_DISTANCE));
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
          //pixels.setPixelColor(i, pixels.Color(blinkColor?255*MAX_INTESITY/100:0,0,0)); 
        }              
      } else {
        for(int i=0;i<numLightPixels;i++){
          int r = 255 * i/NUMPIXELS;
          int g = 255 - r;     
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          leds[i] = CRGB( r*MAX_INTESITY/100,g*MAX_INTESITY/100,0);
          //pixels.setPixelColor(i, pixels.Color(r*MAX_INTESITY/100,g*MAX_INTESITY/100,0)); 
        }
        // Turn off the rest
        for(int i=numLightPixels;i<NUMPIXELS;i++){
          //pixels.setPixelColor(i, pixels.Color(0,0,0)); 
          leds[i] = CRGB( 0, 0, 0);
        }
      }
    }

    FastLED.show(); // This sends the updated pixel color to the hardware.
  }
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per
  // centimeter. The ping travels out and back, so to find 
  //the distance of the object we take half of the distance 
  //travelled.
  return microseconds / 29 / 2;
}
