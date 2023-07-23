#ifndef CALENDARAPP_H
#define CALENDARAPP_H

#include "stdbool.h"

#define MAX_EVENTS 10
#define DEVICE_NAME "PiCalendar"
#define DAYS_TO_OFFSET 5
#define MAX_CALENDAR_AMOUNT 10
#define MAX_LIST_ITEM_AMOUNT 20
#define MAX_SCREEN_WIDTH 800 //785
#define MAX_SCREEN_HEIGHT 475

void feed_wdt();
void stop_custom_wdt();
void start_custom_wdt();

typedef struct {
    char name[256];
    bool completed;
    int total_items;
} list_item_t;

typedef struct {
    bool empty;
    char summary[50];
    char description[100];
    char location[100];
    char start_date[20];
    char end_date[20];
    char start[100];
    char end[100];
} event_t;

typedef struct {
    char name[50];
    char entity_id[100];
    bool empty;
    int event_count;
    event_t events[MAX_EVENTS];
    int calendar_count;
} calendar_t;

#endif // CALENDARAPP_H