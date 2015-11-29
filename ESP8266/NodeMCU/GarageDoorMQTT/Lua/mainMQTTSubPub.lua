BROKER = "192.168.1.116" -- Ip/hostname of MQTT broker
BRPORT = 1883            -- MQTT broker port
BRUSER = ""              -- If MQTT authenitcation is used then define the user
BRPWD  = ""              -- The above user password
CLIENTID = "ESP8266-" ..  node.chipid() -- The MQTT ID. Change to something you like

g_isMQTTConnected = false

-- Switch (input)
doorOpenPin   = 3
doorClosedPin = 2

-- Relay (output)
doorMotorStartPin = 5
AUXPin            = 6

-- DHT22 Temp./Humid
TEMP_DELTA_THRESHOLD = 300 -- = 0.3 Celcius
HUMI_DELTA_THRESHOLD = 3
dht22DataPin    = 7
g_grandPrevTemp = 1
g_intPrevHumi   = 0


sensorSubscriptionDomain = "knx/status/Systemfunksjoner/Sentralfunksjoner/#"

sensorOutput1 = "knx/status/Systemfunksjoner/Sentralfunksjoner/NodeMCU1" -- // Activate Door from KNX - NodeMCU.PutPinHigh
sensorOutput2 = "knx/status/Systemfunksjoner/Sentralfunksjoner/NodeMCU2"
sensorInput1Topic = "knx/set/Statuskontroll/Sentralfunksjoner/NodeMCU_Full_Open"  -- // input1Pin DoorClosedState 
sensorInput2Topic = "knx/set/Statuskontroll/Sentralfunksjoner/NodeMCU_Full_Closed" --// input2Pin DoorOpenState   
sensorInput3Topic = "knx/set/Statuskontroll/Sentralfunksjoner/NodeMCU_Temperature"     --// Temperature
sensorInput4Topic = "knx/set/Statuskontroll/Sentralfunksjoner/NodeMCU_Humidity"        --// Humidity 

--sensorInput1Topic = "knx/set/Statuskontroll/Sentralfunksjoner/Garasjedor Full Open"  -- // input1Pin DoorClosedState 
--sensorInput2Topic = "knx/set/Statuskontroll/Sentralfunksjoner/Garasjedor Full Closed" --// input2Pin DoorOpenState   
--sensorInput3Topic = "knx/set/Statuskontroll/Sentralfunksjoner/Garasje Temperature"     --// Temperature
--sensorInput4Topic = "knx/set/Statuskontroll/Sentralfunksjoner/Garasje Humidity"        --// Humidity 

-- init mqtt client with keepalive timer 120sec
m = mqtt.Client(CLIENTID, 120, BRUSER, BRPWD)

-- setup Last Will and Testament (optional)
-- Broker will publish a message with:
-- qos = 0, retain = 0, data = "offline" 
-- to topic "/lwt" if client don't send keepalive packet
m:lwt("/lwt", "The garage is offline", 0, 0)

m:on("connect", function(con) 
                  print ("Connected") 
                  g_isMQTTConnected = true
                  m:subscribe(sensorSubscriptionDomain, 0, function(m) print("Subscribe /Sentralfunksjoner/#") end )
                end )

m:on("offline", function(con) 
                  print ("offline") 
                  g_isMQTTConnected = false
                end)

-- on publish message receive event
m:on("message", function(conn, topic, data) 
  print(topic .. ":" ) 
  if data ~= nil then
    print(data)
  end

  -- Just a brief contact to activate Garage Door motor
  if topic == sensorOutput1 then
    gpio.write(doorMotorStartPin, gpio.HIGH)    
  end
    
  if topic == sensorOutput2 then
    gpio.write(AUXPin, gpio.HIGH)
  end
  
  tmr.delay(500000)

  gpio.write(doorMotorStartPin, gpio.LOW)
  gpio.write(AUXPin, gpio.LOW)
  
end)

function DoorOpenMakeContact(level)
    print("TriggerOpen ON")
    if inInt then   -- don't allow interrupt in interrupt
        return
    else 
        inInt = true
    end
    tmr.delay(100)  -- debounce
    inInt = false
    
    -- Publish MQTT- event
    m:publish(sensorInput1Topic,"1", 0, 0)
    -- Now, switch trigger to catch up/break
    gpio.trig(doorOpenPin, 'up', DoorOpenBrakContact)
end

function DoorOpenBrakContact(level)
    print("TriggerOpen OFF")
    if inInt then 
        return
    else 
        inInt = true
    end    
    tmr.delay(100) 
    inInt = false

    m:publish(sensorInput1Topic,"0", 0, 0)
    -- Now, switch trigger to catch down/Make
    gpio.trig(doorOpenPin, 'down', DoorOpenMakeContact)
end

function DoorClosedMakeContact(level)
    print("TriggerClose ON")
    if inInt then  
        return
    else 
        inInt = true
    end
    tmr.delay(100) 
    inInt = false
        
    m:publish(sensorInput2Topic,"1", 0, 0)
    gpio.trig(doorClosedPin, 'up', DoorClosedBrakContact)
end

function DoorClosedBrakContact(level)
    print("TriggerClose OFF")
    if inInt then 
        return
    else 
        inInt = true
    end    
    tmr.delay(100) 
    inInt = false

    m:publish(sensorInput2Topic,"0", 0, 0)
    gpio.trig(doorClosedPin, 'down', DoorClosedMakeContact)
end

function ReadDHT22()  

  status, l_intTemp, l_intHumi, l_fractTemp, l_fractHumi = dht.read(dht22DataPin)

  if( status == dht.OK and g_isMQTTConnected ) then

    --l_intTemp = l_intTemp - 1 -- Sensor calibration
    l_strTemp = string.format("%d.%03d", math.floor(l_intTemp), l_fractTemp)
    l_strHumi = string.format("%d.%03d", math.floor(l_intHumi), l_fractHumi)             

    if g_nextReadTemp then 
      -- Temperature
      local l_grandTemp = (math.abs(l_intTemp) * 1000) + l_fractTemp

      -- Only send if change > threshold
      if(math.abs(l_grandTemp - g_grandPrevTemp) >= TEMP_DELTA_THRESHOLD) then
        g_grandPrevTemp = l_grandTemp 
        
        m:publish(sensorInput3Topic,l_strTemp, 0, 0) 
      end 
    else 
      -- Humidity      
      l_intHumiDelta = math.floor(l_intHumi) - g_intPrevHumi

      if( math.abs(l_intHumiDelta) >= HUMI_DELTA_THRESHOLD ) then
        g_intPrevHumi = l_intHumi
        
        --print("1/1/15&value=" .. l_strHumi)                        
        m:publish(sensorInput4Topic,l_strHumi, 0, 0) 
      end      
    end
    g_nextReadTemp = not g_nextReadTemp
  end -- DHT.OK
    
end


-- MAIN{}
-- HW Init
gpio.mode(5, gpio.OUTPUT)
gpio.mode(5, gpio.HIGH)
gpio.mode(6, gpio.OUTPUT)
gpio.mode(6, gpio.HIGH)

-- Setup trigger for DoorOpen switch
gpio.mode(doorOpenPin, gpio.INT, gpio.PULLUP)

-- Assume door is not open at boot
gpio.trig(doorOpenPin, 'down', DoorOpenMakeContact) 

-- Setup trigger for DoorClosed switch
gpio.mode(doorClosedPin, gpio.INT, gpio.PULLUP)

-- Assume door is not open at boot
gpio.trig(doorClosedPin, 'down', DoorClosedMakeContact) 

inInt = false

m:connect(BROKER, BRPORT, 0)

tmr.alarm(2, 1000, 1, function() 
                        ReadDHT22() 
                      end )