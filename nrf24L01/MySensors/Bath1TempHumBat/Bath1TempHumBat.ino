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
 *   Temp - NRF24-set/12/1/1/0/0        1/0/41
 *   Hum  - NRF24-set/12/0/1/0/1        1/0/42
 *   Bat. - NRF24-set/12/255/3/0/0      4/2/21
 */

#define MY_NODE_ID 12
 
// Enable debug prints
// #define MY_DEBUG
  
// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h>  
#include "DHT.h"

#define DHTTYPE DHT22 

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds)

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;

#define VBAT_PER_BITS 0.003363075  
#define VMIN 1.9                                  //  Vmin (radio Min Volt)=1.9V (564v)
#define VMAX 3.0                                  //  Vmax = (2xAA bat)=3.0V (892v)
int batteryPcnt = 0;                              // Calc value for battery %
int batLoop = 0;                                  // Loop to help calc average
int batArray[6];   

DHT dht;
float lastTemp = 0.0;
float lastHum = 0.0;
const double tempDeltaThreshold = 0.2;
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

    #ifdef MY_DEBUG
      Serial.print("T: ");
      Serial.println(temperature);
    #endif
    
    send(msgTemp.set(temperature, 1));
    blink();blink();
    
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
      #ifdef MY_DEBUG
        Serial.print("H: ");
        Serial.println(humidity);
      #endif
    
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      blink();     
  }
  
  batM();
  sleep(SLEEP_TIME); //sleep a bit
}

// Simple sensor feedback
void blink()
{
  digitalWrite(13, HIGH);
  delay(500);            
  digitalWrite(13, LOW); 
}


void batM() //The battery calculations
{
   delay(500);
   // Battery monitoring reading
   int sensorValue = analogRead(BATTERY_SENSE_PIN);    
   delay(500);
   Serial.println(sensorValue);
   // Calculate the battery in %
   float Vbat  = sensorValue * VBAT_PER_BITS;
   int batteryPcnt = static_cast<int>(((Vbat-VMIN)/(VMAX-VMIN))*100.);

   #ifdef MY_DEBUG
   Serial.print("Battery percent: "); Serial.print(batteryPcnt); Serial.println(" %");  
   #endif
   
   // Add it to array so we get an average of 3 (3x20min)
   batArray[batLoop] = batteryPcnt;
   
   if (batLoop > 5) {  
    batteryPcnt = (batArray[0] + batArray[1] + batArray[2] + batArray[3]);
    batteryPcnt = batteryPcnt / 6;
    
    
    if (batteryPcnt > 100) { batteryPcnt=100; }
    
    //Serial.print("Battery Average (Send): "); Serial.print(batteryPcnt); Serial.println(" %");
    //if (oldBatteryPcnt != batteryPcnt) {
    // Power up radio after sleep
    sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
    //}
    
    batLoop = 0;
   }
   else { 
    batLoop++; 
   }
}

