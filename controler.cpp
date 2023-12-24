#ifndef CONTROLER_F
#define CONTROLER_F

#include "registers.cpp"
#include <stdio.h>
#include "pico/stdlib.h"

namespace CONTROLER {
    uint8_t pin_dark_sensor = 15;
    uint8_t pin_esco_1      = 14;
    uint8_t pin_esco_2      = 13;

    uint8_t pin_switch_1_up   = 20;
    uint8_t pin_switch_1_down = 21;
    uint8_t pin_switch_2_up   = 22;
    uint8_t pin_switch_2_down = 26;

    uint8_t relay_valve     = 16;
    uint8_t relay_pump      = 17;
    uint8_t relay_heater    = 18;
    uint8_t relay_co_heater = 19;

    void get_io() {
        DARK_SENSOR = gpio_get(pin_dark_sensor);    
        ESCO_1      = gpio_get(pin_esco_1);
        ESCO_2      = gpio_get(pin_esco_2);

        if (gpio_get(pin_switch_1_up)) {
            SWITCH_1_STATE = 1;
        } else if (gpio_get(pin_switch_1_down)) {
            SWITCH_1_STATE = 2;
        } else {
            SWITCH_1_STATE = 0;
        }

        if (gpio_get(pin_switch_2_up)) {
            SWITCH_2_STATE = 1;
        } else if (gpio_get(pin_switch_2_down)) {
            SWITCH_2_STATE = 2;
        } else {
            SWITCH_2_STATE = 0;
        }
    }

    void setHeater(bool state) {

    }

    void setPomp(bool state) {

    }

    void setValve(bool open) {

    }

    void setCO_Heater(bool state) {

    }


    void determine_group1() {

    }

    void determine_group2() {

    }

    void determine_group3() {

    }

    void determine() {
        get_io();
        if (SWITCH_1_STATE == 1) {
            determine_group1();
        }

        if (SWITCH_2_STATE == 1) {
            determine_group2();
        }

        determine_group3();


    }

    void init() {
        gpio_init     (pin_dark_sensor);
        gpio_set_dir  (pin_dark_sensor, GPIO_IN);
        gpio_set_pulls(pin_dark_sensor, false, true);
        
        gpio_init     (pin_esco_1);
        gpio_set_dir  (pin_esco_1, GPIO_IN);
        gpio_set_pulls(pin_esco_1, false, true);

        gpio_init     (pin_esco_2);
        gpio_set_dir  (pin_esco_2, GPIO_IN);
        gpio_set_pulls(pin_esco_2, false, true);

        gpio_init     (pin_switch_1_up);
        gpio_set_dir  (pin_switch_1_up, GPIO_IN);
        gpio_set_pulls(pin_switch_1_up, false, true);
        
        gpio_init     (pin_switch_1_down);
        gpio_set_dir  (pin_switch_1_down, GPIO_IN);
        gpio_set_pulls(pin_switch_1_down, false, true);

        gpio_init     (pin_switch_2_up);
        gpio_set_dir  (pin_switch_2_up, GPIO_IN);
        gpio_set_pulls(pin_switch_2_up, false, true);

        gpio_init     (pin_switch_2_down);
        gpio_set_dir  (pin_switch_2_down, GPIO_IN);
        gpio_set_pulls(pin_switch_2_down, false, true);

        gpio_init     (relay_valve);
        gpio_set_dir  (relay_valve, GPIO_OUT);
        gpio_set_pulls(relay_valve, false, true);
        
        gpio_init     (relay_pump);
        gpio_set_dir  (relay_pump, GPIO_OUT);
        gpio_set_pulls(relay_pump, false, true);
        
        gpio_init     (relay_heater);
        gpio_set_dir  (relay_heater, GPIO_OUT);
        gpio_set_pulls(relay_heater, false, true);
        
        gpio_init     (relay_co_heater);
        gpio_set_dir  (relay_co_heater, GPIO_OUT);
        gpio_set_pulls(relay_co_heater, false, true);
    }
}

#endif