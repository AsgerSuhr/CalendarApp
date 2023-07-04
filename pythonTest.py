import requests, time
from secrets import *

local_calendar_url = f"http://homeassistant.local:8123/api/calendars"
local_time = time.localtime()
start = f"{local_time[0]:02}-{local_time[1]:02}-{local_time[2]:02}T{local_time[3]:02}:{local_time[4]:02}:{local_time[5]:02}"
end = f"{local_time[0]:02}-{local_time[1]:02}-{local_time[2]+5:02}T{local_time[3]:02}:{local_time[4]:02}:{local_time[5]:02}"
events_filter = f"start={start}&end={end}" 

headers = {
    'Authorization': f"Bearer {access_token}",
    'Content-Type': 'application/json',
    "connection" : "close",
    "user-agent" : "test"
}

calendar = requests.request("GET", local_calendar_url, headers=headers, timeout=5).json()
print(calendar)
print('\n')

for cal in calendar:
    #print(cal)
    cal_url = f"{local_calendar_url}/{cal['entity_id']}?{events_filter}"
    print(cal_url)
    cal_data = requests.request("GET", cal_url, headers=headers, timeout=5)
    print(cal_data.text + "\n")
    #print(cal_data.headers)

null = "null"
test = [{"start":{"date":"2023-07-03"},"end":{"date":"2023-07-05"},"summary":"Test","description":null,"location":null,"uid":"02j3240r6410afq1p920lshhsg@goog\
le.com","recurrence_id":null,"rrule":null},{"start":{"date":"2023-07-04"},"end":{"date":"2023-07-05"},"summary":"filling up","description":null,"locat\
ion":null,"uid":"2kokjfdm0rpusm4q4big7o0teq@google.com","recurrence_id":null,"rrule":null},{"start":{"date":"2023-07-05"},"end":{"date":"2023-07-06"},\
"summary":"Fars fødselsdag 🎉🎉","description":null,"location":null,"uid":"vis2qd25quf9a7p5opmbiub818@google.com","recurrence_id":"vis2qd25quf9a7\
p5opmbiub818_20230705","rrule":"FREQ=YEARLY"},{"start":{"date":"2023-07-05"},"end":{"date":"2023-07-06"},"summary":"busy week","description":null,"loc\
ation":null,"uid":"2e3h2ihpjtd1us16v77q46du9o@google.com","recurrence_id":null,"rrule":null},{"start":{"date":"2023-07-05"},"end":{"date":"2023-07-06"\
},"summary":"so much to do","description":null,"location":null,"uid":"6gl56a16u0o23blrfkbtsf6gq0@google.com","recurrence_id":null,"rrule":null},{"star\
t":{"dateTime":"2023-07-06T18:00:00+02:00"},"end":{"dateTime":"2023-07-06T21:00:00+02:00"},"summary":"Dnd","description":null,"location":null,"uid":"r\
301j2bip9v62ad9k020e9j9ps_R20210906T160000@google.com","recurrence_id":"r301j2bip9v62ad9k020e9j9ps_20230703T160000Z","rrule":null},{"start":{"date":"2\
023-07-07"},"end":{"date":"2023-07-08"},"summary":"prepare for wine friday","description":null,"location":null,"uid":"7qjgvfsrk0q4q35vh8jqh2ar57@googl\
e.com","recurrence_id":null,"rrule":null},{"start":{"date":"2023-07-07"},"end":{"date":"2023-07-10"},"summary":"Stinne og Dinand crasher","description\
":null,"location":null,"uid":"6ss30c31chh64b9gckqm6b9kc4p3abb165gjgbb275j3achi6oq62p1o6o@google.com","recurrence_id":null,"rrule":null},{"start":{"dat\
eTime":"2023-07-08T14:00:00+02:00"},"end":{"dateTime":"2023-07-08T21:00:00+02:00"},"summary":"DnD","description":null,"location":null,"uid":"6gq36o9l6\
koj2bb46sojgb9k74om4b9o69ij4bb2c5i62d9i74o3id356c@google.com","recurrence_id":null,"rrule":null}]

print(test)
