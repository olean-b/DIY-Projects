/**
 * 
 * Custom MySensors node implementing a humidity/temperature
 * sensor using DHT11/DHT-22 
 * 
 * http://www.mysensors.org/build/humidity
 * 
 * Battery monitoring estimates on Arduino Pro Mini:
 * Battery usage 3.3 V @ 8 MHz - No LED
 *  Statup (radio init.) - 17-19mA 
 *  Sleep                -     1mA
 *  Runn   (no radio)    -     9mA
 *  
 *  Node mapping to KNX GroupAdress
 *  ---------------MQTT-----------------KNX GA------------
 *   Temp - NRF24-set/10/1/1/0/0        1/1/14
 *   Hum  - NRF24-set/10/0/1/0/1        1/1/15
 *   Bat. - NRF24-set/10/255/3/0/0      4/2/20
 */

#define MY_NODE_ID 10   

// Enable debug prints
// #define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h>  
#include "dht.h"

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
unsigned long SLEEP_TIME = 60000;  // Sleep time between reads (in milliseconds)

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

dht DHT;
float lastTemp = 0.0;
float lastHum = 0.0;
const double tempDeltaThreshold = 0.2;
const double humDeltaThreshold = 1;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

#define VBAT_PER_BITS 0.003363075  
#define VMIN 1.9                                  //  Vmin (radio Min Volt)=1.9V (564v)
#define VMAX 3.0                                  //  Vmax = (2xAA bat)=3.0V (892v)
int batteryPcnt = 0;                              // Calc value for battery %
int batLoop = 0;                                  // Loop to help calc average
int batArray[6];   

void setup()  
{ 
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
  // Force reading sensor, so it works also after sleep()
  int chk = DHT.read22(HUMIDITY_SENSOR_DIGITAL_PIN);

  if (chk == DHTLIB_OK)
  {
    float temperature = DHT.temperature - 0.5;
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
        temperature = DHT.temperature;
      }
      send(msgTemp.set(temperature, 1));
      blink();blink();
    
      #ifdef MY_DEBUG
        Serial.print("T: ");
        Serial.println(temperature);
      #endif
    }
  
    // Fetch humidity from DHT sensor
    float humidity = DHT.humidity;
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
  }

  batM();
  
  sleep(SLEEP_TIME); //sleep a bit
}

void blink()
{
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
}

void batM() //The battery calculations
{
   delay(500);
   // Battery monitoring reading
   int sensorValue = analogRead(BATTERY_SENSE_PIN);    
   delay(500);
   
   // Calculate the battery in %
   float Vbat  = sensorValue * VBAT_PER_BITS;
   int batteryPcnt = static_cast<int>(((Vbat-VMIN)/(VMAX-VMIN))*100.);

   #ifdef MY_DEBUG
   Serial.print("Battery percent: "); Serial.print(batteryPcnt); Serial.println(" %");  
   #endif
   
   // Add it to array so we get an average of 3 (3x20min)
   batArray[batLoop] = batteryPcnt;
   
   if (batLoop > 5) {  
    batteryPcnt = (batArray[0] + batArray[1] + batArray[2] + batArray[3]+ batArray[4] + batArray[5] + batArray[6]);
    batteryPcnt = batteryPcnt / 6;    
    
    if (batteryPcnt > 100) { 
      batteryPcnt=100; 
      }
    
    //if (oldBatteryPcnt != batteryPcnt) {
      // Power up radio after sleep
      sendBatteryLevel(batteryPcnt);
      oldBatteryPcnt = batteryPcnt;
    //  }
    
    batLoop = 0;
   }
   else { 
    batLoop++; 
   }
}

