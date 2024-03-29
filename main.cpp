#include <stdio.h>

#include "hardware/uart.h"
#include "hardware/structs/spi.h"
#include "hardware/irq.h"
#include "hardware/rtc.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"


#include "registers.cpp"
#include "uart/uart_communication_other_board.cpp"
#include "controler.cpp"

#include "modules/pzem.cpp"
#include "modules/one_wire/api/one_wire.h"
#include "temp.cpp"
#include "modules/24aa01.cpp"

///----USED PINS
///- 0,1,2 UART WITH OTHER BOARD
///- 3     TEMP_ONE_WIRE
///- 4, 5 UART PZEM
///- 6, 7 I2C EEPROM

///- 15 CZUJKA ZMIERZCHU
///- 13 ESCO 1
///- 14 ESCO 2
///- 16 RELAY - zawór
///- 17 RELAY - pompa
///- 18 RELAY - grzałka
///- 19 RELAY - strzelec

//- 20 - PRZEŁĄCZNIK GÓRNY - DOLNA POZYCJA
//- 21 - PRZEŁĄCZNIK GÓRNY - GÓRNA POZYCJA
//- 22 - PRZEŁĄCZNIK DOLNY - DOLNA POZYCJA
//- 26 - PRZEŁĄCZNIK DOLNY - GÓRNA POZYCJA

bool core_2_runnin = true;


void loop_core2() {
    printf("Starting uart");
    UART_BETWEEN_BOARDS::add_dataToGet   (UART_BETWEEN_BOARDS::DataType::TIME);
    UART_BETWEEN_BOARDS::add_dataToGet   (UART_BETWEEN_BOARDS::DataType::HARMONOGRAM);

    UART_BETWEEN_BOARDS::add_dataToReport(UART_BETWEEN_BOARDS::DataType::BOOT);
    UART_BETWEEN_BOARDS::init(true,true,true);
    
    while(1) {
        if (REBOOT_FLAG) {
            core_2_runnin = false;
            return;
        }
        UART_BETWEEN_BOARDS::checkForRequestedData();
        sleep_ms(50);
    };
}

void loop_core1() {
    pzem::init();
    TEMP_ONE_WIRE::init();
    CONTROLER::init();
    // EPROOM_24AA01::init();
    uint8_t counter = 0;
    sleep_ms(5);
    // if (EPROOM_24AA01::checkIfDataPresent()) {
    //     EPROOM_24AA01::readAll();
    // }

    for (int i = 0; i < 5; i++) {
        CONTROLER::determine(true);
    }
    while (true) {
        if (REBOOT_FLAG ) {
            multicore_reset_core1();
            printf("reeboot\n");
            return;
            // watchdog_enable(1, 1);
            // while(1);
        }
        // if (counter > 5) {
            // EPROOM_24AA01::writeAll();
            // counter = 0;
        // }
        counter++;
        int32_t time_since_boot = to_ms_since_boot(get_absolute_time());
        int32_t time_to_wait    = (int32_t)TEMP_ONE_WIRE::startTemp_mesure();
        
        while (1) {
            UART_BETWEEN_BOARDS::executeFunctionsToBeExecuted();
            CONTROLER::determine();
            int32_t new_time_since_boot = to_ms_since_boot(get_absolute_time());
            if (new_time_since_boot - time_since_boot > time_to_wait) {
                break;
            }
        }
        pzem::uartRead();
        // pzem::getHumanReadAbleData();
        // pzem::printData();
        TEMP_ONE_WIRE::saveMesure();

    }
}


int main() {
    stdio_init_all();
    multicore_launch_core1(loop_core2);
    loop_core1();
    watchdog_enable(1, 1);
    return 0;
}