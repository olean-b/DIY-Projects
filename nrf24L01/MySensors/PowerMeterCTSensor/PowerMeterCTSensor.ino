// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3  ***/
#define MY_NODE_ID 9
#define MY_RADIO_NRF24
//#define MY_DEBUG
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>  
#include "EmonLib.h"             // Include Emon Library
EnergyMonitor emon1;             // Create an instance
#define CHILD_ID 1 
#define PIN_ANALOG_I A2

// pulseCount Request = NRF24-set/9/1/2/0/24
// Send pulseCount      NRF24-status/9/1/2/0/24 => Value

unsigned long lastSend;
unsigned long lastSend2;
unsigned long SEND_FREQUENCY = 40000; // Minimum time between send (in milliseconds). We don't wnat to spam the gateway.
unsigned long SEND_FREQUENCY2 = SEND_FREQUENCY / 25;
int index = 0;
double Irms=0;
double power;
boolean pcReceived = false;
boolean onyva=true;
float nrj=0, old_nrj;
MyMessage IrmsMsg(CHILD_ID,V_WATT);
MyMessage kWhMsg(CHILD_ID,V_KWH);
MyMessage pcMsg(CHILD_ID,V_VAR1);


void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Energy Meter", "1.0");

  // Register this device as power sensor
  present(CHILD_ID, S_POWER);
}

void receive(const MyMessage &message) 
{
  if (message.type==V_VAR1) 
  {  
    nrj = old_nrj = message.getFloat();
    Serial.print("Received last nrj count from gw:");
    Serial.println(nrj);
    pcReceived = true;
  }
}

void setup()
{  
  emon1.current(PIN_ANALOG_I, 111.1);       // Current: input pin, calibration.
}

void loop()
{
 // if (onyva) process();
  onyva = false; 
  unsigned long now = millis();
  bool sendTime2 = now - lastSend2 > SEND_FREQUENCY2;
  if (sendTime2) //calc Irms
  {
    if (index==0) Irms=emon1.calcIrms(1480);
    else {    
    index++;
    Irms = (index*Irms+emon1.calcIrms(1480))/(index+1);
    }
    
    lastSend2 = now;
  }
  
  bool sendTime = now - lastSend > SEND_FREQUENCY;
  if (sendTime && pcReceived) 
  { 
    power = Irms*232.0;
    send(IrmsMsg.set(power,1));
    Serial.println(Irms*232.0);
    nrj += (power*SEND_FREQUENCY/1000)/3.6E6;
    send(kWhMsg.set(nrj,5));
    send(pcMsg.set(nrj,5));
    lastSend = now;
    index = 0;
    old_nrj=nrj;
    onyva=true;
  }
 else if (sendTime && !pcReceived)
 {
  request(CHILD_ID, V_VAR1);
  lastSend=now;
  index=0;
  onyva=true;
 }
}

