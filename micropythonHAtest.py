import time
import urequests
from machine import Pin
import secrets 
import network 

wlan = network.WLAN(network.STA_IF)
wlan.active(True)

wlan.connect(secrets.wifi_SSID, secrets.wifi_PASSWORD)
print(wlan.isconnected())

led = Pin("LED", Pin.OUT)

calendar_id = "calendar.calendar"
events_filter = "start=2023-04-02&end=2023-04-01"
access_token = secrets.access_token 
local_calendar_url = f"http://homeassistant.local:8123/api/calendars/{calendar_id}?{events_filter}"
local_calendar_url = f"http://homeassistant.local:8123/api/calendars"
#print(local_calendar_url)

local_time = time.localtime()
start = f"{local_time[0]:02}-{local_time[1]:02}-{local_time[2]:02}T{local_time[3]:02}:{local_time[4]:02}:{local_time[5]:02}"
end = f"{local_time[0]:02}-{local_time[1]:02}-{local_time[2]+1:02}T{local_time[3]:02}:{local_time[4]:02}:{local_time[5]:02}"
events_filter = f"start={start}&end={end}" 
#print(events_filter)
#start=2023-05-21T11:02:43.787457&end=2023-05-28 11:02:43.787457 
headers = {
    'Authorization': f"Bearer {access_token}",
    'Content-Type': 'application/json'
}

calendar = urequests.request("GET", local_calendar_url, headers=headers, timeout=5).json()
#print(calendar)

for cal in calendar:
    #print(cal)
    cal_url = f"{local_calendar_url}/{cal['entity_id']}?{events_filter}"
    print(cal_url)
    led.off()
    cal_data = urequests.request("GET", cal_url, headers=headers, timeout=5)
    print(cal_data.text)
    led.on()
    
led.off()


