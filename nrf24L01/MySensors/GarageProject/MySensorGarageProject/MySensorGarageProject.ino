/**
 * The MySensors Arduino library handles the wireless radio link and protocol
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
 * DESCRIPTION
 *
 */
//// NRF24-status/7/0/1/0/2 - Message 0 or 1
//// NRF24-set/#

/* NodeRed output 6-10
 * NRF24-set/7/1/1/0/0    => "knx/set/Statuskontroll/Sentralfunksjoner/Garasje Temperature"
 * NRF24-set/7/0/1/0/1    => "knx/set/Statuskontroll/Sentralfunksjoner/Garasje Humidity"
 * NRF24-set/7/3/1/0/16   => "knx/set/Statuskontroll/Sentralfunksjoner/Garasjedor Full Closed"  1=blink
 * NRF24-set/7/4/1/0/16   => "knx/set/Statuskontroll/Sentralfunksjoner/Garasjedor Full Open"    1=Const
 * NRF24-status/7/0/1/0/2 => "knx/status/Systemfunksjoner/Sentralfunksjoner/GarasjedorStart"
 */

// Enable debug prints to serial monitor
//#define MY_DEBUG 
#define MY_NODE_ID 7

// Enable and select radio type attached
#define MY_RADIO_NRF24

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <DHT.h>  
#include <MySensors.h>
#include <Bounce2.h>

#define FULL_CLOSED_ID  3 // Full closed
#define FULL_OPEN_ID    4 // Full open

#define FULL_CLOSED_PIN 3  // Arduino Digital I/O pin for button/reed switch
#define FULL_OPEN_PIN   4

#define RELAY_PIN  7  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)

#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 6

Bounce debouncerFClosed = Bounce(); 
Bounce debouncerFOpen = Bounce(); 
int oldFClosedValue=-1;
int oldFOpenValue=-1;
bool relayState;
// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage closeMsg(FULL_CLOSED_ID, V_TRIPPED);
MyMessage openMsg(FULL_OPEN_ID,   V_TRIPPED);

DHT dht;
float lastTemp = 0.0;
float lastHum = 0.0;
const double tempDeltaThreshold = 0.3;
const double humDeltaThreshold = 2;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void setup()  
{ 
  // Setup DHT Temp/Humid
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  metric = getConfig().isMetric;
  
  // Setup the button
  pinMode(FULL_CLOSED_PIN,INPUT);
  pinMode(FULL_OPEN_PIN,INPUT);
  
  // Activate internal pull-up
  digitalWrite(FULL_CLOSED_PIN,HIGH);  
  digitalWrite(FULL_OPEN_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncerFClosed.attach(FULL_CLOSED_PIN);
  debouncerFClosed.interval(5);

  debouncerFOpen.attach(FULL_OPEN_PIN);
  debouncerFOpen.interval(5);

    // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);   
}

void presentation() {
  // Register binary input sensor to gw (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  present(FULL_CLOSED_ID, S_DOOR);  
  present(FULL_OPEN_ID,   S_DOOR);  
  present(RELAY_PIN, S_LIGHT);
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
}

//  Check if digital input has changed and send in new value
void loop() 
{
  readDHT();
  
  debouncerFClosed.update();
  // Get the update value
  int value = debouncerFClosed.read();
 
  if (value != oldFClosedValue) {
     // Send in the new value
     send(closeMsg.set(value==HIGH ? 0 : 1));
     oldFClosedValue = value;
  }

  debouncerFOpen.update();
  // Get the update value
  value = debouncerFOpen.read();
 
  if (value != oldFOpenValue) {
     // Send in the new value
     send(openMsg.set(value==HIGH ? 0 : 1));
     oldFOpenValue = value;
  }  
} 

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }

  Serial.println(message.type);

  if (message.type == V_LIGHT) {
     // Change relay state
     relayState = message.getBool();
     digitalWrite(RELAY_PIN, RELAY_ON);
     // Store state in eeprom
     //saveState(CHILD_ID, relayState);

    #ifdef MY_DEBUG
       Serial.print("Incoming change for sensor:");
       Serial.print(message.sensor);
       Serial.print(", New status: ");
       Serial.println(message.getBool());
     #endif    

     wait(750);
     digitalWrite(RELAY_PIN, RELAY_OFF);
   }
} 

void readDHT()
{
  //delay(dht.getMinimumSamplingPeriod());
 
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
    send(msgTemp.set(temperature, 1));        
      
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
      
      #ifdef MY_DEBUG
        Serial.print("H: ");
        Serial.println(humidity);
      #endif
  }  
}
