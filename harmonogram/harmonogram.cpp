#ifndef HARMONOGRAM_F
#define HARMONOGRAM_F

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "../registers.cpp"

uint8_t DEV_COUNTER_HARMONOGRAM = 0;
uint8_t DEV_HARMONOGRAM [90];
uint8_t ACTIVE_HARMONOGRAM = 0;
const uint8_t MAX_HARMO_AMOUNT = 90;

extern void doReboot();

enum class HARMONOGRAM_ACTIONS {
    NOTHING,
    REBOOT,
    ENABLE_GROUP_1,
    ENABLE_GROUP_2,
    ENABLE_GROUP_3,
    DISABLE_GROUP_1,
    DISABLE_GROUP_2,
    DISABLE_GROUP_3,
};
void setNextHarmonogrameIrq();

void performHarmonogramAction(uint8_t action) {
    switch (action) {
        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::REBOOT):
            printf("WOOOOOOOO HARMONOGRAM REBOOT\n");
            doReboot();
        break;

        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::ENABLE_GROUP_1):
            printf("WOOOOOOOO HARMONOGRAM ENABLE_GROUP_1\n");
            GROUP_1_HARMONOGRAM = 1;
        break;
        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::ENABLE_GROUP_2):
            printf("WOOOOOOOO HARMONOGRAM ENABLE_GROUP_2\n");
            GROUP_2_HARMONOGRAM = 1;
        break;
        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::ENABLE_GROUP_3):
            printf("WOOOOOOOO HARMONOGRAM ENABLE_GROUP_3\n");
            GROUP_3_HARMONOGRAM = 1;
        break;
        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::DISABLE_GROUP_1):
            printf("WOOOOOOOO HARMONOGRAM DISABLE_GROUP_1\n");
            GROUP_1_HARMONOGRAM = 0;
        break;
        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::DISABLE_GROUP_2):
            printf("WOOOOOOOO HARMONOGRAM DISABLE_GROUP_2\n");
            GROUP_2_HARMONOGRAM = 0;
        break;
        case static_cast<unsigned int>(HARMONOGRAM_ACTIONS::DISABLE_GROUP_3):
            printf("WOOOOOOOO HARMONOGRAM DISABLE_GROUP_3\n");
            GROUP_3_HARMONOGRAM = 0;
        break;
      
        default:

        break;
    }

}

void rtcIRQresolver() {
    uint8_t action = DEV_HARMONOGRAM[(ACTIVE_HARMONOGRAM*3)+2];
    printf("Execution of RTC IRQ\n");
    printf("ACTIVE_HARMONOGRAM %i\n", ACTIVE_HARMONOGRAM);
    printf("ACTION %i\n", action);
    performHarmonogramAction(action);
    setNextHarmonogrameIrq();
}

rtc_callback_t rtc_callback = &rtcIRQresolver;



void setIrq(uint8_t index) {
    uint8_t dotw = DEV_HARMONOGRAM[(index*3)] >> 5;
    uint8_t hour = DEV_HARMONOGRAM[(index*3)] & 0b0001'1111;
    uint8_t min  = DEV_HARMONOGRAM[(index*3)+1];
    printf("SETTING IRQ D: %i  %i:%i\n", dotw, hour, min);
    datetime_t irq = {
            .year  = -1,
            .month = -1,
            .day   = -1,
            .dotw  = dotw, // 0 is Sunday, so 5 is Friday
            .hour  = hour,
            .min   = min,
            .sec   = 10
    };
    rtc_set_alarm(&irq, rtc_callback);
    rtc_enable_alarm();
    datetime_t t;
    rtc_get_datetime(&t);
    printf("t: %i   %i:%i %i.%i.%i ",t.dotw,t.hour, t.min, t.day, t.month, t.year);
}

void setNextHarmonogrameIrq() {
    ACTIVE_HARMONOGRAM += 1;
    if (ACTIVE_HARMONOGRAM >= DEV_COUNTER_HARMONOGRAM) ACTIVE_HARMONOGRAM = 0;
    setIrq(ACTIVE_HARMONOGRAM);
}

void newHarmonogramData() {
    busy_wait_us_32(64);
    datetime_t timeNow;
    rtc_get_datetime(&timeNow);
    int16_t time_dst_now = (((timeNow.dotw+1)*24)*60) + (timeNow.hour*60) + timeNow.min;
    printf("new harmo here!\n");
    printf("DONE\n");
    int16_t dstActive = 0;
    for (int i = 0; i < MAX_HARMO_AMOUNT; i+=3) {
        if (DEV_HARMONOGRAM[i+2] == 0) {
            DEV_COUNTER_HARMONOGRAM = i/3;
            break;
        }

        uint8_t dotw = DEV_HARMONOGRAM[i] >> 5;
        uint8_t hour = DEV_HARMONOGRAM[i] & 0b0001'1111;
        uint8_t min  = DEV_HARMONOGRAM[i+1];
        int16_t dst = (((dotw+1)*24)*60) + (hour*60) + min;

        printf("dotw: %i, \n",dotw);
        printf("\ttime: %i:%i, \n", hour, min);
        printf("\taction: %i, \n",DEV_HARMONOGRAM[i+2]);

        dst = time_dst_now-dst;
        printf("VAL HARMO DST: %i, indekx %i\n", dst, i);

        if (dst >= 0) continue;
        dst *= -1;
        printf("negative go forward\n");
        
        if (dstActive == 0) {
            dstActive = dst;
            ACTIVE_HARMONOGRAM = i/3;
            continue;
        } 
        if (dst < dstActive) {
            dstActive = dst;
            ACTIVE_HARMONOGRAM = i/3;
        }
    }
    printf("Final ACTIVE HARMO DST: %i, indekx %i\n", dstActive, ACTIVE_HARMONOGRAM);
    printf("AMOUNT OF HARMOS: %i\n", DEV_COUNTER_HARMONOGRAM);
    if (DEV_COUNTER_HARMONOGRAM == 0) {
        printf("NO HARMOS\n");
        return;
    }

    printf("PERFORMIN LAST HARMO ACIOTN\n");
    if (ACTIVE_HARMONOGRAM == 0) {
        performHarmonogramAction(DEV_HARMONOGRAM[((DEV_COUNTER_HARMONOGRAM-1)*3)+2]);
    } else {
        performHarmonogramAction(DEV_HARMONOGRAM[((ACTIVE_HARMONOGRAM-1)*3)+2]);
    }
    
    setIrq(ACTIVE_HARMONOGRAM);
}

#endif