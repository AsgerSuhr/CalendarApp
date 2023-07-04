#ifndef CALENDARAPP_H
#define CALENDARAPP_H

#include "stdbool.h"

#define MAX_EVENTS 10
#define DEVICE_NAME "PiCalendar"
#define DAYS_TO_OFFSET 5
#define MAX_CALENDAR_AMOUNT 10

typedef struct {
    bool empty;
    char summary[50];
    char description[100];
    char location[100];
    char start_date[20];
    char end_date[20];
    char start[100];
    char end[100];
} Event;

typedef struct {
    char name[50];
    char entity_id[100];
    bool empty;
    int event_count;
    Event events[MAX_EVENTS];
    int calendar_count;
} Calendar;

#endif // CALENDARAPP_H