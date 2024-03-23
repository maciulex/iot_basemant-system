#ifndef CONTROLER_F
#define CONTROLER_F

#include "registers.cpp"
#include <stdio.h>
#include "pico/stdlib.h"

namespace CONTROLER {
    uint8_t pin_esco_1      = 13;
    uint8_t pin_esco_2      = 14;
    uint8_t pin_dark_sensor = 15;

    uint8_t pin_switch_1_down = 20;
    uint8_t pin_switch_1_up   = 21;
    uint8_t pin_switch_2_down = 22;
    uint8_t pin_switch_2_up   = 26;

    uint8_t relay_valve     = 16;
    uint8_t relay_pump      = 17;
    uint8_t relay_heater    = 18;
    uint8_t relay_co_heater = 19;

    uint64_t time_when_heater_was_active = to_ms_since_boot(get_absolute_time());
    uint8_t mins_to_pump_after_heater = 3;
    uint8_t heater_was_active = false;

    uint8_t PUMP_BOILER_STATUS = 0;
    uint8_t HEATER_BOILER_STATUS = 0;
    uint8_t VALVE_BOILER_STATUS = 0;

    #include "inttypes.h"


    void get_io() {
        uint8_t TMP_DARK_SENSOR       = !gpio_get(pin_dark_sensor);    
        uint8_t TMP_ESCO_1            = !gpio_get(pin_esco_1) ;  
        uint8_t TMP_ESCO_2            = !gpio_get(pin_esco_2) ;  
        uint8_t TMP_pin_switch_1_up   = gpio_get (pin_switch_1_up)  ;          
        uint8_t TMP_pin_switch_1_down = gpio_get (pin_switch_1_down);              
        uint8_t TMP_pin_switch_2_up   = gpio_get (pin_switch_2_up)  ;          
        uint8_t TMP_pin_switch_2_down = gpio_get (pin_switch_2_down);

        sleep_ms(200);

        uint8_t TMP2_DARK_SENSOR       = !gpio_get(pin_dark_sensor);    
        uint8_t TMP2_ESCO_1            = !gpio_get(pin_esco_1) ;  
        uint8_t TMP2_ESCO_2            = !gpio_get(pin_esco_2) ;  
        uint8_t TMP2_pin_switch_1_up   = gpio_get (pin_switch_1_up)  ;          
        uint8_t TMP2_pin_switch_1_down = gpio_get (pin_switch_1_down);              
        uint8_t TMP2_pin_switch_2_up   = gpio_get (pin_switch_2_up)  ;          
        uint8_t TMP2_pin_switch_2_down = gpio_get (pin_switch_2_down);

        if (TMP_DARK_SENSOR == TMP2_DARK_SENSOR)
            DARK_SENSOR = TMP2_DARK_SENSOR;    

        if (TMP_ESCO_1 == TMP2_ESCO_1)
            ESCO_1 = TMP2_ESCO_1;    
        
        if (TMP_ESCO_2 == TMP2_ESCO_2)
            ESCO_2 = TMP2_ESCO_2;    

        if (TMP_pin_switch_1_up == TMP2_pin_switch_1_up && TMP_pin_switch_1_down == TMP2_pin_switch_1_down) {
            if (       TMP_pin_switch_1_up) {
                SWITCH_1_STATE = 1;
            } else if (TMP_pin_switch_1_down) {
                SWITCH_1_STATE = 2;
            } else {
                SWITCH_1_STATE = 0;
            }
        }

        if (TMP_pin_switch_2_up == TMP2_pin_switch_2_up && TMP_pin_switch_2_down == TMP2_pin_switch_2_down) {
            if (       TMP_pin_switch_2_up) {
                SWITCH_2_STATE = 1;
            } else if (TMP_pin_switch_2_down) {
                SWITCH_2_STATE = 2;
            } else {
                SWITCH_2_STATE = 0;
            }
        }
    }

    void setHeater(bool state) {
        if (state) {
            gpio_put(relay_heater,0);
            HEATER_BOILER_STATUS = 1;
        } else {
            gpio_put(relay_heater,1);
            if (HEATER_BOILER_STATUS == 1) {
                heater_was_active = true;
                time_when_heater_was_active = to_ms_since_boot(get_absolute_time());
            }
            HEATER_BOILER_STATUS = 0;
        }
    }

    void setPomp(bool state) {
        if (state) {
            gpio_put(relay_pump,0);
            PUMP_BOILER_STATUS = 1;
        } else {
            gpio_put(relay_pump,1);
            PUMP_BOILER_STATUS = 0;
        }
    }

    void setValve(bool open) {
        if (open) {
            gpio_put(relay_valve,0);
            VALVE_BOILER_STATUS = 1;
        } else {
            gpio_put(relay_valve,1);
            VALVE_BOILER_STATUS = 0;
        }
    }

    void setCO_Heater(bool state) {
        if (state) {
            gpio_put(relay_co_heater,0);
        } else {
            gpio_put(relay_co_heater,1);
        }
    }


    void determine_group1() {
        GROUP_1_STATUS = false;
        if (DISABLE_GROUP_1) {
            return;
        }

        if (FORCE_ENABLE_GROUP_1) {
            GROUP_1_STATUS = true;
            return;
        }

        if (GROUP_2_STATUS) {
            GROUP_1_STATUS = false;
            return;
        }

        if (GROUP_1_HARMONOGRAM) {
            GROUP_1_STATUS = true;
            return;
        }

        if (DARK_SENSOR && ESCO_1 && !ESCO_2){
            GROUP_1_STATUS = true;
            return;
        }

        if (HEAT_AND_OFF_GROUP_1) {
            if (TEMP_ONE_WIRE::SensorsData_float[4] == 0xffff) {
                GROUP_1_STATUS = false;
                return;
            }

            if (TEMP_TRESHOLD_FOR_HEAT_AND_OFF < TEMP_ONE_WIRE::SensorsData_float[4]) {
                HEAT_AND_OFF_GROUP_1 = false;
                GROUP_1_STATUS = false;
                return;
            }
            GROUP_1_STATUS = true;
            return;
        }
        GROUP_1_STATUS = false;
    }

    void determine_group2() {
        GROUP_2_STATUS = false;
        if (DISABLE_GROUP_2) {
            return;
        }

        if (FORCE_ENABLE_GROUP_2) {
            GROUP_2_STATUS = true;
            return;
        }

        datetime_t timeNow;
        rtc_get_datetime(&timeNow);

        if (timeNow.hour > 22 || timeNow.hour < 6) {
            GROUP_2_STATUS = false;
            return;
        }

        if (GROUP_2_HARMONOGRAM) {
            GROUP_2_STATUS = true;
            return;
        }

        if ((!ESCO_1 && ESCO_2)){
            GROUP_2_STATUS = true;
            return;
        }
        GROUP_2_STATUS = false; 
    }

    void determine_group3() {
        if (DISABLE_GROUP_3) {
            GROUP_3_STATUS = false;
            return;
        }

        if (FORCE_ENABLE_GROUP_3) {
            GROUP_3_STATUS = true;
            return;
        }

        if (GROUP_3_HARMONOGRAM) {
            GROUP_3_STATUS = true;
            return;
        }

        if (HEAT_AND_OFF_GROUP_3) {
            if (TEMP_ONE_WIRE::SensorsData_float[3] == 0xffff) {
                GROUP_3_STATUS = false;
                return;
            } 

            if (TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF < TEMP_ONE_WIRE::SensorsData_float[3]) {
                HEAT_AND_OFF_GROUP_3 = false;
                GROUP_3_STATUS = false;
                return;
            }
            GROUP_3_STATUS = true;
            return;
        }
        GROUP_3_STATUS = false;
    }

    void determine(bool dont_change_output = false) {
        get_io();

        if (SWITCH_2_STATE == 1) {
            determine_group2();
        } else if (SWITCH_2_STATE == 2) {
            GROUP_2_STATUS = true;
        } else
            GROUP_2_STATUS = false;

        if (SWITCH_1_STATE == 1) {
            determine_group1();
        } else if (SWITCH_1_STATE == 2) {
            GROUP_1_STATUS = true;
        } else
            GROUP_1_STATUS = false;

        determine_group3();

        if (dont_change_output) {
            return;
        }

        if (GROUP_1_STATUS)
            setHeater(true);
        else 
            setHeater(false);

        if (GROUP_2_STATUS)
            setValve (true);
        else 
            setValve (false);


        uint8_t pomp_special_status = false;
        if (heater_was_active) {
            uint32_t time_now = to_ms_since_boot(get_absolute_time());
            //printf("time left: %"PRId32" \n", time_now - time_when_heater_was_active);
            if (time_now - time_when_heater_was_active > ((uint32_t)mins_to_pump_after_heater)*(60*1000)) {
                heater_was_active = false;
            } else {
                pomp_special_status = true;
            }
        }


        if (GROUP_1_STATUS || GROUP_2_STATUS || pomp_special_status) 
            setPomp  (true);
        else 
            setPomp  (false);



        if (GROUP_3_STATUS) 
            setCO_Heater(true);
        else
            setCO_Heater(false);
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