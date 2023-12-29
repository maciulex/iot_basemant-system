#ifndef EPROOM_24AA01_F
#define EPROOM_24AA01_F

#include "hardware/i2c.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"


#include "../registers.cpp"

namespace EPROOM_24AA01 {
    i2c_inst_t * i2c_port = i2c1;
    uint8_t sda_pin = 7;
    uint8_t scl_pin = 6;
    uint16_t boundrate = 100*1000;
    uint8_t sectors = 1024/64;//sequential write work only with max 8 bytes of data then needed to wait for about 4ms to began new write

    uint8_t control_code   = 0b1010'0000>>1;

    uint8_t dataPresentSequence = 0b0101'0111;
    uint8_t * ADDRESS_TO_REGISTERS[] = {&dataPresentSequence, &CONTROLER::mins_to_pump_after_heater, &CONTROLER::heater_was_active,
                                        &TEMP_TRESHOLD_FOR_HEAT_AND_OFF,
                                        &TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF,
                                        &GROUP_1_HARMONOGRAM,
                                        &GROUP_2_HARMONOGRAM,
                                        &GROUP_3_HARMONOGRAM,
                                        &DISABLE_GROUP_1,
                                        &DISABLE_GROUP_2,
                                        &DISABLE_GROUP_3,
                                        &FORCE_ENABLE_GROUP_1,
                                        &FORCE_ENABLE_GROUP_2,
                                        &FORCE_ENABLE_GROUP_3,
                                        &HEAT_AND_OFF_GROUP_1,
                                        &HEAT_AND_OFF_GROUP_2,
                                        &HEAT_AND_OFF_GROUP_3
                                        ,nullptr};
    uint8_t SIZE_OF_REGISTERS[]    = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

    
    void writeRegisters(uint8_t *dataIn, uint8_t len);
    void readRegister(uint8_t *dataIn, uint8_t *dataOut, uint8_t len);
    void randomRead(uint8_t reg_address, uint8_t * dataOut, uint8_t size);

    bool checkIfDataPresent() {
        uint8_t result = 0;
        sleep_ms(4);
        randomRead(0b0000'0000, &result, 1);
        sleep_ms(4);
        if (result == dataPresentSequence) {
            return true;
        }
        return false;
    }

    void randomRead(uint8_t reg_address, uint8_t * dataOut, uint8_t size) {
        i2c_write_blocking(i2c_port, control_code, &reg_address, 1, false);
        sleep_ms(4);
        printf("readed: %i\n",i2c_read_blocking(i2c_port, control_code, dataOut, size, false));
    }

    void clearAllMemory() {
        printf("Clearing EEPROM DATA\n");
        uint8_t data[9];
        for (int i = 0; i < 9; i++) 
            data[i] = 0;
        for (int i = 0; i < sectors; i++) {
            data[0] = i*8;
            i2c_write_blocking(i2c_port, control_code, data, 9, false);
            sleep_ms(4);
        }
    }        

    void readAll() {
        uint8_t final_size = 1;

        uint8_t register_counter = 0;
        while (ADDRESS_TO_REGISTERS[register_counter])
            final_size += SIZE_OF_REGISTERS[register_counter++];
        
        uint8_t * readed = new uint8_t [final_size];
        randomRead(0,readed, final_size);

        register_counter = 1;
        uint8_t buffer_counter = 1;

        while (ADDRESS_TO_REGISTERS[register_counter]) {
            for (int i = 0; i < SIZE_OF_REGISTERS[register_counter]; i++) {
                ADDRESS_TO_REGISTERS[register_counter][i] = readed[buffer_counter++];
            }

            register_counter += 1;
        }

    }

    void writeAll() {
        printf("\tSTARt data write\n");
        
        uint8_t sleep_time = 10;

        uint8_t current_address = 0;
        uint8_t register_counter = 0;
        uint8_t buffer[9];
        uint8_t buffer_counter = 1;
        while (ADDRESS_TO_REGISTERS[register_counter]) {
            buffer[0] = current_address;

            for (int i = 0; i < SIZE_OF_REGISTERS[register_counter]; i++) {
                buffer[buffer_counter++] = ADDRESS_TO_REGISTERS[register_counter][i];

                if (buffer_counter >= 9) {
                    i2c_write_blocking(i2c_port, control_code, buffer, 9, false);
                    for (int r = 0; r < 9; r++) buffer[r] = 0;
                    current_address += 8;
                    buffer[0] = current_address;
                    buffer_counter = 1;
                    sleep_ms(sleep_time);
                }
            }

            register_counter++;
        }
        if (buffer_counter > 1) {
            i2c_write_blocking(i2c_port, control_code, buffer, 9, false);
            sleep_ms(sleep_time);
        }
        printf("Writting all to eprom\n");

    }

    void readRegister(uint8_t *dataIn, uint8_t *dataOut, uint8_t len) {
        i2c_write_blocking(i2c_port, control_code, dataIn, 1, false);
        i2c_read_blocking (i2c_port, control_code, dataOut, len, false);
    }

    void init_i2c() {
        i2c_init(i2c_port, boundrate);
        gpio_set_function(sda_pin, GPIO_FUNC_I2C);
        gpio_set_function(scl_pin, GPIO_FUNC_I2C);
        gpio_pull_up(sda_pin);
        gpio_pull_up(scl_pin);
        bi_decl(bi_2pins_with_func(sda_pin, scl_pin, GPIO_FUNC_I2C));
    }

    void init(bool init_i2c_b = true) {
        if (init_i2c_b) {
            init_i2c();
        }
    }



}



#endif