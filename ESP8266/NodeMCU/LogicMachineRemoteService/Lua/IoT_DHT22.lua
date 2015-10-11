TEMP_DELTA_THRESHOLD = 300 -- = 0.3 Celcius
HUMI_DELTA_THRESHOLD = 3

TEMP_OBJ_ADR         = "1/1/14"         -- Group Address (GA) of Temperature object (09. 2 byte ploating point)
HUMI_OBJ_ADR         = "1/1/15"         -- Group Address (GA) of Humidity object (09. 2 byte ploating point)
SCADA_REMOTE_IP      = "192.168.1.123"  -- IP of LM
SCADA_AUTH_USR_PASS  = "remote:remote"  -- [UserName:Password] or LM remote service

DHT22_DATA_PIN       = 4                -- DataPin GPIO2
RESTART_COUNT_LIMIT  = 100

g_intRestartCount = 0 
g_grandPrevTemp   = 1
g_intPrevHumi     = 0
g_nextReadTemp    = true

function ReadDHT22()  
  status, l_intTemp, l_intHumi, l_fractTemp, l_fractHumi = dht.read(DHT22_DATA_PIN)

  if( status == dht.OK ) then
    l_strTemp = string.format("%d.%03d", math.floor(l_intTemp), l_fractTemp)
    l_strHumi = string.format("%d.%03d", math.floor(l_intHumi), l_fractHumi)             

    if g_nextReadTemp then 
      -- Temperature
      local l_grandTemp = (math.abs(l_intTemp) * 1000) + l_fractTemp

      -- Only send if change > threshold
      if(math.abs(l_grandTemp - g_grandPrevTemp) >= TEMP_DELTA_THRESHOLD) then
        g_grandPrevTemp = l_grandTemp 

        print("1/1/14&value=" .. l_strTemp)
        --writeLMValue(TEMP_OBJ_ADR .. "&value=" .. l_strTemp)          
      end 
    else 
      -- Humidity      
      l_intHumiDelta = math.floor(l_intHumi) - g_intPrevHumi

      if( math.abs(l_intHumiDelta) >= HUMI_DELTA_THRESHOLD ) then
        g_intPrevHumi = l_intHumi
        
        print("1/1/15&value=" .. l_strHumi)                        
        --writeLMValue(HUMI_OBJ_ADR .. "&value=" .. l_strHumi)
      end      
    end
    g_nextReadTemp = not g_nextReadTemp
  end -- DHT.OK
    
end

function writeLMValue(aliasValue)

   conn=net.createConnection(net.TCP, 0) 

   conn:on("receive", function(conn, payload) 
                        conn:close()
                      end ) 

   conn:on("disconnection", function(conn)
                              g_intRestartCount = g_intRestartCount + 1

                              if( g_intRestartCount >= RESTART_COUNT_LIMIT) then
                                node.Restart()
                              end
                            end)
  
   conn:connect(80, SCADA_REMOTE_IP) 
   conn:send("GET /cgi-bin/scada-remote/request.cgi?m=json&r=grp&fn=write&alias=" .. aliasValue .." HTTP/1.1\r\n"..
               "Host: " .. SCADA_REMOTE_IP .. "\r\n"..
               "Accept: */*\r\n"..
               "User-Agent: Mozilla/4.0 (compatible; esp8266 NodeMcu Lua;)\r\n"..            
               "Authorization: Basic " .. crypto.toBase64(SCADA_AUTH_USR_PASS) .. "\r\n"..
               "\r\n") 
end

-- read data every 60 sek =  60000
tmr.alarm(2, 6000, 1, function() ReadDHT22() end )
