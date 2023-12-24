#ifndef TEMP_F
#define TEMP_F


#include "pico/stdlib.h"
#include "modules/one_wire/api/one_wire.h"
namespace TEMP_ONE_WIRE {
    const uint8_t dataPin = 3;
    uint8_t amountOfDevices = 0;
    
    One_wire one_wire(dataPin); 
    rom_address_t address {};
    
    uint8_t SensorsData[20];
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
        amountOfDevices = one_wire.find_and_count_devices_on_bus();
        for (int z = 0; z < 5; z++) {
            SensorsData_float[z] = 0xffff;
        }
        for (int z = 0; z < 20; z++) {
            SensorsData[z] = 0xff;
        }
        for (int i = 0; i < amountOfDevices; i++) {
            address = one_wire.get_address(i);
            float res = one_wire.temperature(address);


            uint64_t a = one_wire.to_uint64(address);
            switch (a) {
                case 0x2857795704e13c47://niebieski 1
                    SensorsData[0]  = ((((uint64_t)res) >> 24) & 0xff);
                    SensorsData[1]  = ((((uint64_t)res) >> 16) & 0xff);
                    SensorsData[2]  = ((((uint64_t)res) >> 8)  & 0xff);
                    SensorsData[3]  = ( ((uint64_t)res) & 0xff);
                    SensorsData_float[0] = res;
                break;
                case 0x28246f80e3e13c5c://niebieski 2
                    SensorsData[4]  = ((((uint64_t)res) >> 24) & 0xff);
                    SensorsData[5]  = ((((uint64_t)res) >> 16) & 0xff);
                    SensorsData[6]  = ((((uint64_t)res) >> 8)  & 0xff);
                    SensorsData[7]  = ( ((uint64_t)res) & 0xff);
                    SensorsData_float[1] = res;
                break;
                case 0x28ae5280e3e13c11://Brązowy   1
                    SensorsData[8]  = ((((uint64_t)res) >> 24) & 0xff);
                    SensorsData[9]  = ((((uint64_t)res) >> 16) & 0xff);
                    SensorsData[10] = ((((uint64_t)res) >> 8)  & 0xff);
                    SensorsData[11] = ( ((uint64_t)res) & 0xff);
                    SensorsData_float[2] = res;
                break;
                case 0x286ee180e3e13c8f://Brązowy   2
                    SensorsData[12] = ((((uint64_t)res) >> 24) & 0xff);
                    SensorsData[13] = ((((uint64_t)res) >> 16) & 0xff);
                    SensorsData[14] = ((((uint64_t)res) >> 8)  & 0xff);
                    SensorsData[15] = ( ((uint64_t)res) & 0xff);
                    SensorsData_float[3] = res;
                break;
                case 0x28003458d4e13c6a://Brązowy   3 //
                    SensorsData[16] = ((((uint64_t)res) >> 24) & 0xff);
                    SensorsData[17] = ((((uint64_t)res) >> 16) & 0xff);
                    SensorsData[18] = ((((uint64_t)res) >> 8)  & 0xff);
                    SensorsData[19] = ( ((uint64_t)res) & 0xff);
                    SensorsData_float[4] = res;
                break;
            }
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