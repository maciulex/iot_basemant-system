#ifndef ON_DATA_SET_FUNCTIONS_F
#define ON_DATA_SET_FUNCTIONS_F

#include "modules/pzem.cpp"
#include "modules/one_wire/api/one_wire.h"
#include "temp.cpp"
#include "controler.cpp"

void RESET_ONE_WIRE_EXE_F() {
    printf("reset one wire\n");
    TEMP_ONE_WIRE::init();
}

void RESET_PZEM_F() {
    printf("reset pzem\n");
    pzem::reset();
}

void reset_pump() {
    CONTROLER::heater_was_active = false;
}

void resetHarmo_flags() {
    printf("Clearing harmo flags\n");
    GROUP_1_HARMONOGRAM = false;
    GROUP_2_HARMONOGRAM = false;
    GROUP_3_HARMONOGRAM = false;
}
#endif