WIFI_SSID = "MySSID"
WIFI_PASS = "MyWiFiPass"
g_WiFiConnectTry = 0

wifi.setmode(wifi.STATION)

wifi.sta.config(WIFI_SSID, WIFI_PASS)
wifi.sta.connect()

tmr.alarm( 1, 1000, 1, function() 

  if wifi.sta.getip() == nil then 
    print("IP unavaiable, Waiting...") 
    g_WiFiConnectTry = g_WiFiConnectTry + 1
    
    if g_WiFiConnectTry >= 20 then
      node.restart()
    end
  else 
    tmr.stop(1)
    print("Config done, IP is "..wifi.sta.getip())
    dofile("mainMQTTSubPub.lua")
  end 

end)
