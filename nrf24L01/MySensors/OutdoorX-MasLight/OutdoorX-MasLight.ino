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
//// NRF24-status/6/0/1/0/2 - Message 0 or 1
//// NRF24-set/#

/* NodeRed output 6-10
 * NRF24-status/7/0/1/0/2 => "knx/status/Statuskontroll/1. etg/Utelys, Status"
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 
#define MY_NODE_ID 6

// Enable and select radio type attached
#define MY_RADIO_NRF24

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>

#define RELAY_PIN  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)

#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define CHILD_ID 1   // Id of the sensor child

bool relayState;
MyMessage msg(CHILD_ID,V_LIGHT);

void setup()  
{ 
  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);   
}

void presentation() {
  present(RELAY_PIN, S_LIGHT);
}

//  Check if digital input has changed and send in new value
void loop() 
{

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
     saveState(CHILD_ID, relayState);

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

