#ifndef TIME_F
#define TIME_F

#include "hardware/rtc.h"
#include "pico/util/datetime.h"

void newTimeFunc();

uint8_t DEV_TIME_RAW[8];

datetime_t date_time;


void newTimeFunc() {
    if (!rtc_running()){
        rtc_init();
    }

    datetime_t newTime  = {
            .year  = (DEV_TIME_RAW[3]*100)+DEV_TIME_RAW[4],
            .month = DEV_TIME_RAW[5],
            .day   = DEV_TIME_RAW[6],
            .dotw  = DEV_TIME_RAW[7], // 0 is Sunday, so 5 is Friday
            .hour  = DEV_TIME_RAW[0],
            .min   = DEV_TIME_RAW[1],
            .sec   = DEV_TIME_RAW[2]
    };

    printf(
        "NEW DATE: %i.%i.%i, %i:%i\n", newTime.day, newTime.month, newTime.year, newTime.hour, newTime.min
    );

    printf("DAY WEEK: %i, sec: %i\n", newTime.dotw, newTime.sec);

    rtc_set_datetime(&newTime);
    

}



#endif