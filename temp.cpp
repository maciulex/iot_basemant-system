#ifndef TEMP_F
#define TEMP_F


#include "pico/stdlib.h"
#include "modules/one_wire/api/one_wire.h"
namespace TEMP_ONE_WIRE {
    const uint8_t dataPin = 3;
    uint8_t amountOfDevices = 0;
    
    One_wire one_wire(dataPin); 
    rom_address_t address {};
    
    uint8_t SensorsData[10];
    float   SensorsData_float[5];

    int startTemp_mesure() {
        return one_wire.convert_temperature(address, false, true);
    }

    //niebieski 1 supły 2857795704e13c47 piec in
    //niebieski 2 supły 28246f80e3e13c5c piec out
    //Brązowy   1 supły 28ae5280e3e13c11 strzelec in
    //Brązowy   2 supły 286ee180e3e13c8f strzelec out
    //Brązowy   3 supły 28003458d4e13c6a bojler

    void saveMesure() {
        bool setted[5] = {false,false,false,false,false};
        
        for (int i = 0; i < amountOfDevices; i++) {
            address = one_wire.get_address(i);
            float res = one_wire.temperature(address);

            uint64_t a = one_wire.to_uint64(address);
            int8_t   integer_part = (int8_t )   res;
            uint8_t  real_part1   = (uint8_t) ((res - integer_part)*100);

            switch (a) {
                case 0x2857795704e13c47://niebieski 1
                    SensorsData[0] = (uint8_t)integer_part;
                    SensorsData[1] = real_part1;
                    SensorsData_float[0] = res;
                    setted[0] = true;
                break;
                case 0x28246f80e3e13c5c://niebieski 2
                    SensorsData[2] = (uint8_t)integer_part;
                    SensorsData[3] = real_part1;
                    SensorsData_float[1] = res;
                    setted[1] = true;
                break;
                case 0x28ae5280e3e13c11://Brązowy   1
                    SensorsData[4] = (uint8_t)integer_part;
                    SensorsData[5] = real_part1;
                    SensorsData_float[2] = res;
                    setted[2] = true;
                break;
                case 0x286ee180e3e13c8f://Brązowy   2
                    SensorsData[6] = (uint8_t)integer_part;
                    SensorsData[7] = real_part1;
                    SensorsData_float[3] = res;
                    setted[3] = true;
                break;
                case 0x28003458d4e13c6a://Brązowy   3 //
                    SensorsData[8] = (uint8_t)integer_part;
                    SensorsData[9] = real_part1;
                    SensorsData_float[4] = res;
                    setted[4] = true;
                break;
            }
        }
        for (int z = 0; z < 5; z++) {
            if (setted[z]) continue;
            SensorsData[ z*2]    = 0xff;
            SensorsData[(z*2)+1] = 0xff;
            SensorsData_float[4] = 0xffff;
        }
    }

    void init() {
        one_wire.init();
        amountOfDevices = one_wire.find_and_count_devices_on_bus();
        for (int i = 0; i < amountOfDevices; i++) {
            one_wire.set_resolution(one_wire.get_address(i), 12);
        }
    }
}
#endif