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

#include "libs/pzem.cpp"

#ifndef INT_TO_BIN
#define INT_TO_BIN
    #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
    #define BYTE_TO_BINARY(byte)  \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0') 
#endif

void sendDataToPython();
void core2Loop();
void core1Loop();
void rtcIRQresolver();
uint8_t processRecivedData();
void setHarmonogrameIrq();
//ACTING AS SLAVE!
// #define SPI_COMMUNICATION_WITH_PYTHON_PORT spi1
// //SPI1_IRQ SPI0_IRQ
// #define SPI_COMMUNICATION_WITH_PYTHON_IRQ_PORT SPI1_IRQ

// #define SPI_COMMUNICATION_WITH_PYTHON_SPEED 1000*1000

// #define SPI_COMMUNICATION_WITH_PYTHON_RX  12
// #define SPI_COMMUNICATION_WITH_PYTHON_CSN 13
// #define SPI_COMMUNICATION_WITH_PYTHON_SCK 14
// #define SPI_COMMUNICATION_WITH_PYTHON_TX  15
#define UART_BOUND_RATE 9600
#define UART_PORT_IRQ UART0_IRQ
#define UART_PORT uart0
#define UART_TX_PIN 12
#define UART_RX_PIN 13

#define PYTHON_SERVER_PACKAGE_SIZE 50

bool secondCoreSended = false;
uint8_t DATA_TO_PYTHON[PYTHON_SERVER_PACKAGE_SIZE] {};
uint8_t DATA_FROM_PYTHON[PYTHON_SERVER_PACKAGE_SIZE] {};
uint8_t uart_error_counter = 0;

//max 2 harmonograms for each day , after good data is guard == 0xff'ff
short unsigned int daysOfTheHarmonograme     [2] {0xff'ff};
short unsigned int actionOfTheHarmonograme   [2] {0xff'ff};
short unsigned int startHourOfTheHarmonograme[2] {0xff'ff};
short unsigned int endHourOfTheHarmonograme  [2] {0xff'ff};

bool CONTROLER_GROUP_1 = false;
bool CONTROLER_GROUP_2 = false;
bool HARMONOGRAME_GROUP_1 = false;
bool HARMONOGRAME_GROUP_2 = false;
bool FORCED_ON_GROUP_1 = false;
bool FORCED_ON_GROUP_2 = false;
bool newsToCore1ToSetHarmonograme = false;
struct harmonograme_t
{   
    //index for 2 demension of array if == 2 that means that it is last irq of the day allways at 23:59
    uint8_t TodayHarmonogrameIndex = 0;
    bool isStartedAlready = false;

    uint8_t day[7][2] {
        {255,255},{255,255},
        {255,255},{255,255},
        {255,255},{255,255},
        {255,255},
    };
    uint8_t action[7][2] {
        {255,255},{255,255},
        {255,255},{255,255},
        {255,255},{255,255},
        {255,255},
    };

    uint8_t startTime[7][2][2] {
        {{255,255},{255,255}},{{255,255},{255,255}},
        {{255,255},{255,255}},{{255,255},{255,255}},
        {{255,255},{255,255}},{{255,255},{255,255}},
        {{255,255},{255,255}},
    };
    //layers// days -> slot for day -> {hours, minutes}
    uint8_t endTime[7][2][2] {
        {{255,255},{255,255}},{{255,255},{255,255}},
        {{255,255},{255,255}},{{255,255},{255,255}},
        {{255,255},{255,255}},{{255,255},{255,255}},
        {{255,255},{255,255}},
    };
} ;
harmonograme_t HARMONOGRAME;

rtc_callback_t rtcCallback = &rtcIRQresolver;
irq_handler_t SPIO1_IRQ_HANDLER = &sendDataToPython;
//(4)10, (5)11 ALLWAYS HIGHT
//(0)16 - Zawór, 0 - otwarty, 1 - zamkniety
//(1)17 - Pompa, 1 - wył, 0 - wł
//(2)18 - Grzałka, 1 - wył, 0 - wł ! 19 też musi być jednakowe
//(3)19 - Grzałka, 1 - wył, 0 - wł ! 18 też musi być jednakowe
//(6)27 <- pytanie do micropythona o date i godzine
//(7)26 <- pytanie do micropythona o charmonogram
uint8_t OUTPUTS[] {16,17,18,19,10,11,27,26,255};

#define AMOUNT_OF_INPUTS 8
//grupa 1 -> grzałka + pompa
//grupa 2 -> pompa + zawór
//(0)2 - (1)3 <- grupa 1 | (0)2 == 1 < sterownik | (1)3 == 1 < wł cały czas | (0)2 == (1)3 == 0 < wył całkowicie
//(2)4 - (3)5 <- grupa 2 | (2)4 == 1 < sterownik | (3)5 == 1 < wł cały czas | (2)4 == (3)5 == 0 < wył całkowicie
//(4)6 <- sygnał sterujący z esco | hight sterownik mówi by grupa 1 była wyłączona
//(5)7 <- sygnał sterujący z esco | hight sterownik mówi by grupa 2 była wyłączona 
//(4)6 - 5(7) nigdy nie są razem wysokie
//(6)8 - czujnik zmierzchowy wysoki jeżeli jest mało światła
//(7)20-jeżeli wysoki to flagi dateToSet i harmonograme to false
uint8_t INPUTS [] {2,3,4,5,6,7,21,20,255};
bool INPUTS_STATES[AMOUNT_OF_INPUTS]; 
bool flagForSignalToResetData = false;
bool dateToSet          = true;
bool harmonogrameToSet  = true;


datetime_t timeFromServer = {
        .year  = 2020,
        .month = 06,
        .day   = 05,
        .dotw  = 5, // 0 is Sunday, so 5 is Friday
        .hour  = 15,
        .min   = 45,
        .sec   = 30
};

void rtcIRQresolver() {
    datetime_t timeNow;
    rtc_get_datetime(&timeNow);
    int8_t day = timeNow.day;
    printf("RESOLVER index %i, day: %i\n", HARMONOGRAME.TodayHarmonogrameIndex, day);
    if (HARMONOGRAME.TodayHarmonogrameIndex == 2) {
        printf("Resolver for 23:59 trigger\n");
        HARMONOGRAME.TodayHarmonogrameIndex = 0;
        HARMONOGRAME.isStartedAlready = false;
        printf("waiting for new day\n");
        while (day == timeNow.day) {
            rtc_get_datetime(&timeNow);
            busy_wait_ms(1000);
        }
        printf("new day come\n");
        
        setHarmonogrameIrq();
        printf("Setting harmonograme For new day\n");
        return;
    }

    if (HARMONOGRAME.isStartedAlready) {
        if (HARMONOGRAME.action[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex] == 1) {
            HARMONOGRAME_GROUP_1 = false;
            printf("deACTIVATING GROUP 1\n");
        }else if (HARMONOGRAME.action[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex] == 2) {
            HARMONOGRAME_GROUP_2 = false;
            printf("deACTIVATING GROUP 2\n");
        }
        HARMONOGRAME.TodayHarmonogrameIndex += 1;
        HARMONOGRAME.isStartedAlready = false;
        setHarmonogrameIrq();
    } else {
        if (HARMONOGRAME.action[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex] == 1) {
            HARMONOGRAME_GROUP_1 = true;
            printf("ACTIVATING GROUP 1\n");
        }else if (HARMONOGRAME.action[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex] == 2) {
            HARMONOGRAME_GROUP_2 = true;
            printf("ACTIVATING GROUP 2\n");
        }
        HARMONOGRAME.isStartedAlready = true;
        setHarmonogrameIrq();
    }
}

void setHarmonogrameIrq() {
    datetime_t timeNow;
    rtc_get_datetime(&timeNow);
    int8_t hour = 0;
    int8_t min  = 0;
    while (true){
        printf("SETTING IRQ FOR DAY: %i irqIndex: %i\n", timeNow.dotw, HARMONOGRAME.TodayHarmonogrameIndex);
        if (HARMONOGRAME.TodayHarmonogrameIndex == 2) {
            printf("rtcIRQ setted 23:59 bc no more interupts\n");
            hour = 23;
            min  = 59;
            HARMONOGRAME.isStartedAlready       = false;
            break;
        }

        if (!HARMONOGRAME.isStartedAlready){
            hour = HARMONOGRAME.startTime[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex][0];
            min  = HARMONOGRAME.startTime[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex][1];
        } else {
            hour = HARMONOGRAME.endTime[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex][0];
            min  = HARMONOGRAME.endTime[timeNow.dotw][HARMONOGRAME.TodayHarmonogrameIndex][1];
        }

        if (hour == 255) {
            printf("rtcIRQ setted 23:59 bc no more interupts\n");
            hour = 23;
            min  = 59;
            HARMONOGRAME.isStartedAlready       = false;
            HARMONOGRAME.TodayHarmonogrameIndex = 2;
            break;
        }
        if (hour < timeNow.hour || (hour == timeNow.hour && min <= timeNow.min)) {
            printf("rtcIRQ skipping time already passed\n");
            HARMONOGRAME.TodayHarmonogrameIndex += 1;
            HARMONOGRAME.isStartedAlready = false;
        } else {
            printf("INTERUPT SETTED %i:%i\n",hour,min);
            break;
        }
    }
    datetime_t timeForRTC = {
        .year  = -1,
        .month = -1,
        .day   = -1,
        .dotw  = -1, // 0 is Sunday, so 5 is Friday
        .hour  = hour,
        .min   = min,
        .sec   = 50
    };
    rtc_set_alarm(&timeForRTC, rtcCallback);
    rtc_enable_alarm();
}

void setNothingToSend() {
    bool second = false;
    for (int i = 0; i < PYTHON_SERVER_PACKAGE_SIZE; i++) {
        if (second) {
            DATA_TO_PYTHON[i] = 0xFE;
        } else {
            DATA_TO_PYTHON[i] = 0xFF;
        }
        second != second;
    }
}

void clearRecivedData() {
    for (int i = 0; i < PYTHON_SERVER_PACKAGE_SIZE; i++) {
        DATA_FROM_PYTHON[i] = 0x00;
    }
}
void initUart() {
    printf("uart declaration\n");
    uart_init(UART_PORT, UART_BOUND_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_PORT, false, false);
    uart_set_format(UART_PORT, 8, 1, UART_PARITY_NONE);
    bi_decl(bi_2pins_with_func(UART_RX_PIN, UART_TX_PIN, GPIO_FUNC_UART));


    irq_set_exclusive_handler(UART_PORT_IRQ, SPIO1_IRQ_HANDLER);
    irq_set_enabled(UART_PORT_IRQ, true);
    uart_set_irq_enables(UART_PORT, true, false);
}

void reinitUart() {
    uart_deinit(UART_PORT);
    busy_wait_ms(1);
    initUart();
}

void softwareReset() {
    watchdog_enable(1,false);
    while(1);
}

void sendDataToPython() {
    datetime_t timeNow;
    rtc_get_datetime(&timeNow);
    DATA_TO_PYTHON[2] = 0xF5;
    DATA_TO_PYTHON[3] = CONTROLER_GROUP_1;
    DATA_TO_PYTHON[4] = CONTROLER_GROUP_2;
    DATA_TO_PYTHON[5] = HARMONOGRAME_GROUP_1;
    DATA_TO_PYTHON[6] = HARMONOGRAME_GROUP_2;
    DATA_TO_PYTHON[7] =  INPUTS_STATES[0];
    DATA_TO_PYTHON[8] =  INPUTS_STATES[1];
    DATA_TO_PYTHON[9] =  INPUTS_STATES[2];
    DATA_TO_PYTHON[10] = INPUTS_STATES[3];
    DATA_TO_PYTHON[11] = INPUTS_STATES[4];
    DATA_TO_PYTHON[12] = INPUTS_STATES[5];
    DATA_TO_PYTHON[13] = INPUTS_STATES[6];

    DATA_TO_PYTHON[14] = timeNow.day;
    DATA_TO_PYTHON[15] = timeNow.month;
    DATA_TO_PYTHON[16] = timeNow.hour;
    DATA_TO_PYTHON[17] = timeNow.min;
    DATA_TO_PYTHON[18] = timeNow.sec;
    DATA_TO_PYTHON[19] = (timeNow.year%1000);
    DATA_TO_PYTHON[20] = FORCED_ON_GROUP_1;
    DATA_TO_PYTHON[21] = FORCED_ON_GROUP_2;

    DATA_TO_PYTHON[22] = pzem::REGISTERS_LAST[3];//voltage
    DATA_TO_PYTHON[23] = pzem::REGISTERS_LAST[4];
    DATA_TO_PYTHON[24] = pzem::REGISTERS_LAST[7];//Amps
    DATA_TO_PYTHON[25] = pzem::REGISTERS_LAST[8];
    DATA_TO_PYTHON[26] = pzem::REGISTERS_LAST[5];
    DATA_TO_PYTHON[27] = pzem::REGISTERS_LAST[6];
    DATA_TO_PYTHON[28] = pzem::REGISTERS_LAST[11];//Wats
    DATA_TO_PYTHON[29] = pzem::REGISTERS_LAST[12];
    DATA_TO_PYTHON[30] = pzem::REGISTERS_LAST[9];
    DATA_TO_PYTHON[31] = pzem::REGISTERS_LAST[10];
    DATA_TO_PYTHON[32] = pzem::REGISTERS_LAST[15];//WatsHours
    DATA_TO_PYTHON[33] = pzem::REGISTERS_LAST[16];
    DATA_TO_PYTHON[34] = pzem::REGISTERS_LAST[13];
    DATA_TO_PYTHON[35] = pzem::REGISTERS_LAST[14];
    DATA_TO_PYTHON[36] = pzem::REGISTERS_LAST[17];//Frequency
    DATA_TO_PYTHON[37] = pzem::REGISTERS_LAST[18];
    DATA_TO_PYTHON[38] = pzem::REGISTERS_LAST[19];//PowerFactor
    DATA_TO_PYTHON[39] = pzem::REGISTERS_LAST[20];
    DATA_TO_PYTHON[40] = pzem::lastCorrect;



    DATA_TO_PYTHON[49] = 0x7A;
    DATA_TO_PYTHON[48] = 0x85;

    printf("blocking of uart interupt\n");
    uart_read_blocking(UART_PORT, DATA_FROM_PYTHON, PYTHON_SERVER_PACKAGE_SIZE);
    uart_write_blocking(UART_PORT, DATA_TO_PYTHON, PYTHON_SERVER_PACKAGE_SIZE);

    printf("going to write\n");
    printf("data sent\n");
    if (DATA_FROM_PYTHON[49] + DATA_FROM_PYTHON[48] != 255) {
        printf("uncorrect data ): uart\n");
        reinitUart();
        uart_error_counter++;
        if (uart_error_counter > 5) {
            softwareReset();
        }
    }
    secondCoreSended = true;

    for (int i = 0; i < 50; i++){
        printf("%i, ", DATA_FROM_PYTHON[i]);
    }


    //spi_read_blocking(SPI_COMMUNICATION_WITH_PYTHON_PORT, 0x01, DATA_FROM_PYTHON, PYTHON_SERVER_PACKAGE_SIZE);
    //spi_write_blocking(SPI_COMMUNICATION_WITH_PYTHON_PORT, DATA_TO_PYTHON, PYTHON_SERVER_PACKAGE_SIZE);
    //spi_write_read_blocking(SPI_COMMUNICATION_WITH_PYTHON_PORT, DATA_TO_PYTHON, DATA_FROM_PYTHON, PYTHON_SERVER_PACKAGE_SIZE);
    //spi_write_read_blocking(SPI_COMMUNICATION_WITH_PYTHON_PORT, DATA_TO_PYTHON, DATA_FROM_PYTHON, PYTHON_SERVER_PACKAGE_SIZE);
}

void core2() {
    // spi_init(SPI_COMMUNICATION_WITH_PYTHON_PORT, SPI_COMMUNICATION_WITH_PYTHON_SPEED);

    // spi_set_slave(SPI_COMMUNICATION_WITH_PYTHON_PORT, true);
    // spi_set_format(SPI_COMMUNICATION_WITH_PYTHON_PORT,  8, spi_cpol_t::SPI_CPOL_1, spi_cpha_t::SPI_CPHA_1, spi_order_t::SPI_MSB_FIRST);

    // gpio_set_function(SPI_COMMUNICATION_WITH_PYTHON_RX,  GPIO_FUNC_SPI);
    // gpio_set_function(SPI_COMMUNICATION_WITH_PYTHON_TX,   GPIO_FUNC_SPI);
    // gpio_set_function(SPI_COMMUNICATION_WITH_PYTHON_SCK,  GPIO_FUNC_SPI);
    // gpio_set_function(SPI_COMMUNICATION_WITH_PYTHON_CSN,  GPIO_FUNC_SPI);

    // spi1_hw->imsc = 1 << 2;
    // irq_set_exclusive_handler(SPI_COMMUNICATION_WITH_PYTHON_IRQ_PORT, SPIO1_IRQ_HANDLER);
    // irq_set_enabled(SPI_COMMUNICATION_WITH_PYTHON_IRQ_PORT, true);
    initUart();

    printf("going to core 2 loop\n");
    core2Loop();
}

void initOutputs() {
    uint8_t counter = 0;
    while (OUTPUTS[counter] != 255) {
        printf("init out %i\n", OUTPUTS[counter]);
        gpio_init(OUTPUTS[counter]);
        gpio_set_dir(OUTPUTS[counter], GPIO_OUT);
        gpio_put(OUTPUTS[counter], 1);
        counter++;
    }
}

void initInputs() {
    uint8_t counter = 0;
    while (INPUTS[counter] != 255) {
        printf("init in %i\n", INPUTS[counter]);
        gpio_init(INPUTS[counter]);
        gpio_set_dir(INPUTS[counter], GPIO_IN);
        gpio_pull_down(INPUTS[counter]);
        counter++;
    }
}

void getInputsStates() {
    bool tmp_state[AMOUNT_OF_INPUTS];
    bool valid[AMOUNT_OF_INPUTS];
    for (int i = 0; i < AMOUNT_OF_INPUTS; i++) 
        tmp_state[i] = gpio_get(INPUTS[i]);
    

    sleep_ms(50);

    for (int i = 0; i < AMOUNT_OF_INPUTS; i++) 
        if (tmp_state[i] == gpio_get(INPUTS[i])) valid[i] = true;

    for (int i = 0; i < AMOUNT_OF_INPUTS; i++) {
        if (!valid[i]) continue;
        INPUTS_STATES[i] = tmp_state[i];
    }
}

void dateGot() {
    dateToSet = false;
    gpio_put(OUTPUTS[6], 1);
}

void getDate() {
    gpio_put(OUTPUTS[6], 0);
}

void harmonogrameGot() {
    gpio_put(OUTPUTS[7], 1);
    harmonogrameToSet = false;
    printf("harmonogram got\n");
}

void getHarmonograme() {
    gpio_put(OUTPUTS[7], 0);
}

uint8_t processRecivedData() {
    uint8_t counter;
    switch (DATA_FROM_PYTHON[0]) {
        case 0x01:
            timeFromServer.year  = (DATA_FROM_PYTHON[4]*100) + DATA_FROM_PYTHON[5];
            timeFromServer.month =  DATA_FROM_PYTHON[6];
            timeFromServer.day   =  DATA_FROM_PYTHON[7];
            timeFromServer.dotw  =  DATA_FROM_PYTHON[8];
            timeFromServer.hour  =  DATA_FROM_PYTHON[1];
            timeFromServer.min   =  DATA_FROM_PYTHON[2];
            timeFromServer.sec   =  DATA_FROM_PYTHON[3];
            rtc_set_datetime(&timeFromServer);
            sleep_us(64);            

            printf("\n\n");
            printf("RCT SETTED \n");
            printf("date: %i.%i.%i\n", timeFromServer.day,timeFromServer.month,timeFromServer.year);
            printf("time: %i:%i:%i\n", timeFromServer.sec,timeFromServer.min,timeFromServer.hour);
            dateGot();
            return 0xFC;
        case 0x02:
            harmonogrameGot();
            HARMONOGRAME = {};
            counter = 1;

            printf("\n\n");
            while (DATA_FROM_PYTHON[counter] != 0) {
                uint8_t day    =          (DATA_FROM_PYTHON[counter]/10);
                uint8_t action =          (DATA_FROM_PYTHON[counter]%10);
                uint8_t timeStartHours   = DATA_FROM_PYTHON[counter+1];
                uint8_t timeStartMinutes = DATA_FROM_PYTHON[counter+2];
                uint8_t timeEndHours     = DATA_FROM_PYTHON[counter+3];
                uint8_t timeEndMinutes   = DATA_FROM_PYTHON[counter+4];
                uint8_t slotIndex = 0;
                if (HARMONOGRAME.day[day][0] != 255) slotIndex = 1;

                HARMONOGRAME.day[day][slotIndex]    = day;
                HARMONOGRAME.action[day][slotIndex] = action;

                HARMONOGRAME.startTime[day][slotIndex][0] = timeStartHours;
                HARMONOGRAME.startTime[day][slotIndex][1] = timeStartMinutes;

                HARMONOGRAME.endTime[day][slotIndex][0] = timeEndHours;
                HARMONOGRAME.endTime[day][slotIndex][1] = timeEndMinutes;

                printf("Counter: %i\n", counter);
                printf("\tDzień: %i, akcja %i\n", day, action);
                printf("\tCzas Start: %i:%i, Czas Koniec %i:%i\n", timeStartHours, timeStartMinutes, timeEndHours, timeEndMinutes);
                counter += 5;
            }
            newsToCore1ToSetHarmonograme = true;
            return 0xFB;
        case 0x03:
            FORCED_ON_GROUP_1 = DATA_FROM_PYTHON[1];
            FORCED_ON_GROUP_2 = DATA_FROM_PYTHON[2];
            printf("SETTED FORCED, F1: %i, F2: %i", FORCED_ON_GROUP_1, FORCED_ON_GROUP_2);
            return 0xFA;
    }
    return 0x00;
}




void setHeater(bool state) {
    gpio_put(OUTPUTS[2],!state);
    gpio_put(OUTPUTS[3],!state);
}
void setPomp(bool state) {
    gpio_put(OUTPUTS[1],!state);
}
void setValve(bool state) {
    gpio_put(OUTPUTS[0],!state);
}

int main() {
    stdio_init_all();
    setNothingToSend();
    sleep_ms(1000);

    rtc_init();
    rtc_set_datetime(&timeFromServer);
    sleep_us(64);

    initOutputs();
    initInputs();
    pzem::init();

    multicore_launch_core1(core2);
    core1Loop();
    return 0;
}

void core2Loop() {
    while (true) {
        if (secondCoreSended) {
            secondCoreSended = false;
            printf("processing data\n");       
            //SET FLAG THAT NEXT REQUEST IS COMMING TO SOON
            DATA_TO_PYTHON[0] = 0xFD;
            DATA_TO_PYTHON[1] = 0xFD;
            DATA_TO_PYTHON[2] = 0xFD;
            uint8_t result = processRecivedData();
            DATA_TO_PYTHON[0] = result;
            clearRecivedData();
            continue;
        }
        //DO SOME STUFF 
        //getHarmonograme();

    }
}

void core1Loop() {
    getDate();
    getHarmonograme();

    INPUTS;
    uint8_t counter = 0;
    while (true) {  
        if (counter > 20) {
            pzem::uartRead();   
            counter = 0;
        }
        counter++;
        sleep_ms(400);  
        getInputsStates();
        if (newsToCore1ToSetHarmonograme) {
            setHarmonogrameIrq();
            newsToCore1ToSetHarmonograme = false;
            printf("CORE 1 SET HARMONOGRAME\n");
        }
        if (INPUTS_STATES[7] && !flagForSignalToResetData) {
            flagForSignalToResetData = true;
            getDate();
            getHarmonograme();
            printf("Trying to set again harmonograme\n");
        } else if (!INPUTS_STATES[7]){
            flagForSignalToResetData = false;
        }

        bool group1 = false;
        bool group2 = false;


        //group 1
        if (INPUTS_STATES[0]) {
            //sterownik
            if ((!INPUTS_STATES[4]) && (!INPUTS_STATES[6])) {
                group1 = true;
            }
            if (HARMONOGRAME_GROUP_1){
                group1 = true;
            }
            if (FORCED_ON_GROUP_1) {
                group1 = true;
            }
        } else if (INPUTS_STATES[1]) {
            group1 = true;
        }
        CONTROLER_GROUP_1 = group1;

        //group 2
        if (INPUTS_STATES[2]) {
            //sterownik
            if (!INPUTS_STATES[5]) {
                group2 = true;
            }
            if (HARMONOGRAME_GROUP_2){
                group2 = true;
            }
            if (FORCED_ON_GROUP_2) {
                group2 = true;
            }
        } else if (INPUTS_STATES[3])
            group2 = true;
        
        CONTROLER_GROUP_2 = group2;
        
        setHeater(group1);
        setValve (group2);
        setPomp  (group1 | group2);

    }
}