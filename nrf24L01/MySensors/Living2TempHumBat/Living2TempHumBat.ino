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
 * Version 1.0 - Henrik EKblad
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a humidity/temperature
 * sensor using DHT11/DHT-22 
 * http://www.mysensors.org/build/humidity
 */

// Temp - NRF24-set/10/1/1/0/0
// Hum  - NRF24-set/10/0/1/0/1
// Bat. - NRF24-set/10/255/3/0/0
 
// Enable debug prints
// #define MY_DEBUG
#define MY_NODE_ID 10   

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
unsigned long SLEEP_TIME = 45000; // Sleep time between reads (in milliseconds)

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

DHT dht;
float lastTemp = 0.0;
float lastHum = 0.0;
const double tempDeltaThreshold = 0.3;
const double humDeltaThreshold = 1;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void setup()  
{ 
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  metric = getConfig().isMetric;

  #if defined(__AVR_ATmega2560__)
   analogReference(INTERNAL1V1);
  #else
   analogReference(INTERNAL);
  #endif
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
}

void loop()      
{  
  delay(dht.getMinimumSamplingPeriod());
 
  // Fetch temperatures from DHT sensor
  float temperature = dht.getTemperature() - 0.5;
  if (isnan(temperature)) 
  {
    #ifdef MY_DEBUG
    Serial.println("Failed reading temperature from DHT");
    #endif
  } 
  else if( fabs(temperature - lastTemp) > tempDeltaThreshold)
  {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.set(temperature, 1));
    blink();blink();
    
    #ifdef MY_DEBUG
      Serial.print("T: ");
      Serial.println(temperature);
    #endif
  }
  
  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();
  if (isnan(humidity)) 
  {
    #ifdef MY_DEBUG
      Serial.println("Failed reading humidity from DHT");
    #endif
  } 
  else if( fabs(humidity - lastHum) > humDeltaThreshold)
  {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      blink();
      
      #ifdef MY_DEBUG
        Serial.print("H: ");
        Serial.println(humidity);
      #endif
  }

  TestBattery();
  
  sleep(SLEEP_TIME); //sleep a bit
}

void blink()
{
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
}

void TestBattery()
{
  // NRF24-set/10/255/3/0/0 => Bat.Pct

   // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
   #ifdef MY_DEBUG
   Serial.println(sensorValue);
   #endif
   
   // 1M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
   // 3.44/1023 = Volts per bit = 0.003363075   
   int batteryPcnt = sensorValue / 10;

   #ifdef MY_DEBUG
   float batteryV  = sensorValue * 0.003363075;
   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
   #endif

   if (oldBatteryPcnt != batteryPcnt) {
     // Power up radio after sleep
     sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }
}

