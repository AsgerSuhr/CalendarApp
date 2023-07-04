#include "datetimes.h"


void datetime_to_today(char *buf, uint buf_size, const datetime_t *t) {
    snprintf(buf,
             buf_size,
             "%s %d %s %d",
             DATETIME_DOWS[t->dotw],
             t->day,
             DATETIME_MONTHS[t->month - 1],
             t->year);
};

bool isLeapYear(int year) {
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0) {
                return true; // Divisible by 400: leap year
            } else {
                return false; // Divisible by 100 but not 400: not a leap year
            }
        } else {
            return true; // Divisible by 4 but not 100: leap year
        }
    } else {
        return false; // Not divisible by 4: not a leap year
    }
}

void offset_datetime(datetime_t *t_ptr, uint8_t offset_days) {

    for (int i=0; i < offset_days; i++) {
        // offsetting the date
        t_ptr->day += 1;
        t_ptr->dotw += 1;

        // making sure we know whether we are in a leap year or not
        uint8_t february_days = 28;
        if (isLeapYear(t_ptr->year)) february_days = 29;

        // making sure we are in the correct month
        if (t_ptr->month == 2 && t_ptr->day > february_days) {
            t_ptr->month += 1;
            t_ptr->day = 1;
        } else if (t_ptr->day > 30 && (t_ptr->month == 4 || t_ptr->month == 6 || t_ptr->month == 9 || t_ptr->month == 11)) {
            t_ptr->month += 1;
            t_ptr->day = 1;
        } else if (t_ptr->day > 31) {
            t_ptr->month += 1;
            t_ptr->day = 1;
        }

        // making sure we are in the correct year
        if (t_ptr->month > 12) {
            t_ptr->year += 1;
            t_ptr->month = 1;
            t_ptr->day = 1;
        }

        // get the right day of the week
        t_ptr->dotw = t_ptr->dotw % 7;
    }

}