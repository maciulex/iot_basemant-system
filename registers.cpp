#ifndef REGISTERS_F
#define REGISTERS_F

#include <stdio.h>
#include "hardware/rtc.h"


//build in registers

bool REBOOT_FLAG = false;
uint8_t DEV_ERRORS [5];
uint8_t DEV_STATUS = 0;

//controler registers

uint8_t SWITCH_1_STATE = 0; // 0 off, 1 controler, 2 allways_on
uint8_t SWITCH_2_STATE = 0; // 0 off, 1 controler, 2 allways_on


uint8_t DARK_SENSOR = false;
uint8_t ESCO_1 = false;
uint8_t ESCO_2 = false;

uint8_t TEMP_TRESHOLD_FOR_HEAT_AND_OFF = 60;
uint8_t TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF = 32;

uint8_t GROUP_1_STATUS = false;
uint8_t GROUP_2_STATUS = false;
uint8_t GROUP_3_STATUS = false;

uint8_t GROUP_1_HARMONOGRAM = false;
uint8_t GROUP_2_HARMONOGRAM = false;
uint8_t GROUP_3_HARMONOGRAM = false;

uint8_t DISABLE_GROUP_1             = false;
uint8_t DISABLE_GROUP_2             = false;
uint8_t DISABLE_GROUP_3             = false;

uint8_t FORCE_ENABLE_GROUP_1 = false;
uint8_t FORCE_ENABLE_GROUP_2 = false;
uint8_t FORCE_ENABLE_GROUP_3 = false;

uint8_t HEAT_AND_OFF_GROUP_1 = false;
uint8_t HEAT_AND_OFF_GROUP_2 = false;
uint8_t HEAT_AND_OFF_GROUP_3 = false;

#include "harmonogram/harmonogram.cpp"
#include "time/time.cpp"
#include "temp.cpp"
#include "OnDataSet_functions.cpp"

    //[0] 0-4 godzina, 5,6,7 - dzien tygodnia, [1] minuty, [3] akcja
#ifdef TIME_F
    void (* DEV_NEW_TIME_FUNC)() = newTimeFunc;
#endif

#ifdef HARMONOGRAM_F
    void (* DEV_NEW_HARMONOGRAM_FUNC)() = newHarmonogramData;
    //[0] 0-4 godzina, 5,6,7 - dzien tygodnia, [1] minuty, [3] akcja
#endif






void doReboot() {
    REBOOT_FLAG = true;
}











#endif