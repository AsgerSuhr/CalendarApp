import requests, time
from secrets import *

local_calendar_url = f"http://homeassistant.local:8123/api/calendars"
local_time = time.localtime()
start = f"{local_time[0]:02}-{local_time[1]:02}-{local_time[2]:02}T{local_time[3]:02}:{local_time[4]:02}:{local_time[5]:02}"
end = f"{local_time[0]:02}-{local_time[1]:02}-{local_time[2]+1:02}T{local_time[3]:02}:{local_time[4]:02}:{local_time[5]:02}"
events_filter = f"start={start}&end={end}" 

headers = {
    'Authorization': f"Bearer {access_token}",
    'Content-Type': 'application/json',
    "connection" : "close",
    "user-agent" : "test"
}

calendar = requests.request("GET", local_calendar_url, headers=headers, timeout=5).json()
print(calendar)

for cal in calendar:
    #print(cal)
    cal_url = f"{local_calendar_url}/{cal['entity_id']}?{events_filter}"
    print(cal_url)
    cal_data = requests.request("GET", cal_url, headers=headers, timeout=5)
    print(cal_data.text + "\n")
    #print(cal_data.headers)

