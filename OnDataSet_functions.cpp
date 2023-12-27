#ifndef ON_DATA_SET_FUNCTIONS_F
#define ON_DATA_SET_FUNCTIONS_F

#include "modules/pzem.cpp"
#include "modules/one_wire/api/one_wire.h"
#include "temp.cpp"

void RESET_ONE_WIRE_EXE_F() {
    TEMP_ONE_WIRE::init();
}

void RESET_PZEM_F() {
    pzem::reset();
}


#endif