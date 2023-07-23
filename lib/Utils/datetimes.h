#ifndef DATETIMES_HEADER
#define DATETIMES_HEADER

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "hardware/watchdog.h"
#include "CalendarApp.h"

static const char *DATETIME_MONTHS[12] = {
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December"
};

static const char *DATETIME_DOWS[7] = {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
};

void sleep_callback(void);
void rtc_sleep(int minutes);
void datetime_to_today(char *buf, uint buf_size, const datetime_t *t);
bool isLeapYear(int year);
void offset_datetime(datetime_t *t_ptr, uint8_t offset_days);
void offset_datetime_minutes(datetime_t *t_ptr, int minutes);

#endif // DATETIMES_HEADER