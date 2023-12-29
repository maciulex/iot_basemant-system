#ifndef UARD_BETWEEN_BOARDS_F
#define UARD_BETWEEN_BOARDS_F

#include <stdio.h>
#include <vector>

#include "hardware/uart.h"
#include "hardware/rtc.h"
#include "pico/binary_info.h"

#include "crc16.cpp"
#include "../registers.cpp"
#include "../controler.cpp"

#include "../modules/pzem.cpp"
#include "../modules/24aa01.cpp"
namespace UART_BETWEEN_BOARDS {
    bool debug = true;
    
    //uart conf
    uint         BOUND_RATE= 9600;
    uint         UART_IRQ  = UART0_IRQ; //UART0_IRQ      UART1_IRQ
    uart_inst_t *UART_PORT = uart0;     //uart0          uart1
    uint8_t TX_PIN = 0;
    uint8_t RX_PIN = 1;
    uint8_t RQ_PIN = 2;



    const uint8_t UART_DATA_WRITE = 0b1000'0000;
    const uint8_t UART_DATA_READ  = 0b0000'0000;



    uint8_t ADDRESS_ME = 0x22;
    uint uart_error_counter = 0;

    bool    PACKAGE_RECIVED = false;
    uint8_t *DATA_OUT  = new uint8_t[6];
    uint8_t *DATA_IN   = new uint8_t[6];


    //data




    struct DATA
    {
        uint8_t code;
        uint8_t * DATA_POINTER;
        uint8_t size;
        void * funcToExecuteWhenNew;
        bool writable;
        DATA(uint8_t c, uint8_t * d_pointer, uint8_t s, bool writableA ,void * toExe) {
            code = c;
            DATA_POINTER = d_pointer;
            size = s;
            funcToExecuteWhenNew = toExe;
            writable = writableA;
        }
    };

    enum DataType {
        NOTHING         = 0b0000'0000,
        ERROR_CODE      = 0b0000'0001,
        STATUS          = 0b0000'0010,
        BOOT            = 0b0000'0011,
        OK              = 0b0000'0100,
        BAD             = 0b0000'0101,
        SEND_ALL        = 0b0000'0110,
        TIME            = 0b0000'0111,
        HARMONOGRAM     = 0b0000'1000,
        REBOOT          = 0b0000'1001,
        
        TEMPS_ONE_WIRE_E = 0b0000'1010,
        PZEM_E           = 0b0000'1011,
        DARK_SENSOR_E,
        ESCO_1_E,
        ESCO_2_E,
        RESET_ONE_WIRE_E,
        RESET_PZEM_E,
        TEMP_TRESHOLD_FOR_HEAT_AND_OFF_E,
        GROUP_1_STATUS_E,
        GROUP_2_STATUS_E,
        GROUP_3_STATUS_E,
        GROUP_1_HARMONOGRAM_E,
        GROUP_2_HARMONOGRAM_E,
        GROUP_3_HARMONOGRAM_E,
        DISABLE_GROUP_1_E,
        DISABLE_GROUP_2_E,
        DISABLE_GROUP_3_E,
        FORCE_ENABLE_GROUP_1_E,
        FORCE_ENABLE_GROUP_2_E,
        FORCE_ENABLE_GROUP_3_E,
        HEAT_AND_OFF_GROUP_1_E,
        HEAT_AND_OFF_GROUP_2_E,
        HEAT_AND_OFF_GROUP_3_E,
        TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF_E,
        SWITCH_1_STATE_E,
        SWITCH_2_STATE_E,

        PUMP_BOILER_STATUS_E,
        HEATER_BOILER_STATUS_E,
        VALVE_BOILER_STATUS_E,
        DELAY_HEATER_PUMP_E,
        HEATER_WAS_ACTIVE_E,

        CLEAR_EEPROM_E,
        CLEAR_HARMO_FLAGS_E
        //DEFINE DEPENDING ON SITUATION
    };
    #include "../OnDataSet_functions.cpp"

    DATA DATA_REFS[] = {
        DATA(DataType::NOTHING        ,  nullptr                   , 0   ,false,  nullptr),
        DATA(DataType::ERROR_CODE     ,  DEV_ERRORS                , 5   ,false,  nullptr),
        DATA(DataType::STATUS         , &DEV_STATUS                , 1   ,false,  nullptr),
        DATA(DataType::BOOT           ,  nullptr                   , 0   ,false,  nullptr),
        DATA(DataType::OK             ,  nullptr                   , 0   ,false,  nullptr),
        DATA(DataType::BAD            ,  nullptr                   , 0   ,false,  nullptr),
        DATA(DataType::SEND_ALL       ,  nullptr                   , 0   ,false,  nullptr),
        DATA(DataType::TIME           ,  DEV_TIME_RAW              , 8   ,true ,  (void *)DEV_NEW_TIME_FUNC       ),
        DATA(DataType::HARMONOGRAM    ,  DEV_HARMONOGRAM           , 90  ,true ,  (void *)DEV_NEW_HARMONOGRAM_FUNC),
        DATA(DataType::REBOOT         ,  nullptr                   , 0   ,true ,  (void *)doReboot),
        
        DATA(DataType::TEMPS_ONE_WIRE_E ,  TEMP_ONE_WIRE::SensorsData  , 10  ,false,  nullptr),
        DATA(DataType::PZEM_E           ,  pzem::REGISTERS_LAST        , 25  ,false,  nullptr),
        DATA(DataType::DARK_SENSOR_E                   ,&DARK_SENSOR   ,1    ,false,  nullptr),
        DATA(DataType::ESCO_1_E                        ,&ESCO_1        ,1    ,false,  nullptr),
        DATA(DataType::ESCO_2_E                        ,&ESCO_2        ,1    ,false,  nullptr),
        DATA(DataType::RESET_ONE_WIRE_E                ,nullptr        ,0    ,true,   (void *) RESET_ONE_WIRE_EXE_F),
        DATA(DataType::RESET_PZEM_E                    ,nullptr        ,0    ,true,   (void *) RESET_PZEM_F),
        DATA(DataType::TEMP_TRESHOLD_FOR_HEAT_AND_OFF_E,&TEMP_TRESHOLD_FOR_HEAT_AND_OFF,1,true,nullptr),
        DATA(DataType::GROUP_1_STATUS_E                ,&GROUP_1_STATUS      , 1, false,nullptr),
        DATA(DataType::GROUP_2_STATUS_E                ,&GROUP_2_STATUS      , 1, false,nullptr),
        DATA(DataType::GROUP_3_STATUS_E                ,&GROUP_3_STATUS      , 1, false,nullptr),
        DATA(DataType::GROUP_1_HARMONOGRAM_E           ,&GROUP_1_HARMONOGRAM , 1, false,nullptr),
        DATA(DataType::GROUP_2_HARMONOGRAM_E           ,&GROUP_2_HARMONOGRAM , 1, false,nullptr),
        DATA(DataType::GROUP_3_HARMONOGRAM_E           ,&GROUP_3_HARMONOGRAM , 1, false,nullptr),
        DATA(DataType::DISABLE_GROUP_1_E               ,&DISABLE_GROUP_1     , 1, true, nullptr),
        DATA(DataType::DISABLE_GROUP_2_E               ,&DISABLE_GROUP_2     , 1, true, nullptr),
        DATA(DataType::DISABLE_GROUP_3_E               ,&DISABLE_GROUP_3     , 1, true, nullptr),
        DATA(DataType::FORCE_ENABLE_GROUP_1_E          ,&FORCE_ENABLE_GROUP_1, 1, true, nullptr),
        DATA(DataType::FORCE_ENABLE_GROUP_2_E          ,&FORCE_ENABLE_GROUP_2, 1, true, nullptr),
        DATA(DataType::FORCE_ENABLE_GROUP_3_E          ,&FORCE_ENABLE_GROUP_3, 1, true, nullptr),
        DATA(DataType::HEAT_AND_OFF_GROUP_1_E          ,&HEAT_AND_OFF_GROUP_1, 1, true, nullptr),
        DATA(DataType::HEAT_AND_OFF_GROUP_2_E          ,&HEAT_AND_OFF_GROUP_2, 1, true, nullptr),
        DATA(DataType::HEAT_AND_OFF_GROUP_3_E          ,&HEAT_AND_OFF_GROUP_3, 1, true, nullptr),
        DATA(DataType::TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF_E,&TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF,1,true,nullptr),
        DATA(DataType::SWITCH_1_STATE_E,                     &SWITCH_1_STATE,1,false,nullptr),
        DATA(DataType::SWITCH_2_STATE_E,                     &SWITCH_2_STATE,1,false,nullptr),

        DATA(DataType::PUMP_BOILER_STATUS_E            ,&CONTROLER::PUMP_BOILER_STATUS,  1,false,nullptr),
        DATA(DataType::HEATER_BOILER_STATUS_E,          &CONTROLER::HEATER_BOILER_STATUS,1,false,nullptr),
        DATA(DataType::VALVE_BOILER_STATUS_E,           &CONTROLER::VALVE_BOILER_STATUS ,1,false,nullptr),
        DATA(DataType::DELAY_HEATER_PUMP_E,             &CONTROLER::mins_to_pump_after_heater,1,true,nullptr),
        DATA(DataType::HEATER_WAS_ACTIVE_E,             &CONTROLER::heater_was_active   ,1,false,nullptr),

        DATA(DataType::CLEAR_EEPROM_E,                  nullptr  ,0,true,(void *)EPROOM_24AA01::clearAllMemory),
        DATA(DataType::CLEAR_HARMO_FLAGS_E,             nullptr  ,0,true,(void *)resetHarmo_flags),
    };

    std::vector<uint8_t> FUNCTIONS_TO_EXECUTE;

    std::vector<uint8_t> DATA_TO_REPORT;
    std::vector<uint8_t> DATA_TO_GET;

    enum ERROR_CODES {
        FATAL_ERROR,
        DATE_DEVIATION,
        DATA_DEVIATION,
        UART_DEVIATION,
        SENSOR_ERROR
    };


    uint8_t MESS_ALL_OK[]  = {ADDRESS_ME, 0x04, DataType::OK , 0, 0};
    uint8_t MESS_ALL_BAD[] = {ADDRESS_ME, 0x06, DataType::BAD, 0, 0};


    void checkForRequestedData();

    void init(bool initUartF = true, bool initIRQ = false, bool rqPin = true);
    void initUart();
    void initUartIRQ();
    void uart_deinit();
    void uart_reinit();
    
    // void newCustomData(uint8_t code, void * recivedData, uint8_t recivedDataSize, void * responseData = nullptr, uint8_t responseDataSize = 0) {
    //     struct CUSTOM_DATA s;
    //     s.code              = code | 0b1000'0000;
    //     s.DATA_POINTER      = recivedData;
    //     s.size              = recivedDataSize;
    //     CUSTOM.push_back(s);
    // }

    void clearUart() {
        while (uart_is_readable_within_us(UART_PORT, 500)) {
            uart_getc(UART_PORT);
        }
    }

    void add_dataToGet(DataType d) {
        DATA_TO_GET   .push_back(d);
    }

    void add_dataToReport(DataType d) {
        DATA_TO_REPORT.push_back(d);
    }

    void defineInBufferSizes(uint8_t in) {
        delete [] DATA_IN;
        DATA_IN = new uint8_t [in];
    }

    void defineOutBufferSizes(uint8_t out) {
        delete [] DATA_OUT;
        DATA_OUT   = new uint8_t [out];
    }

    void defineBufferSizes(uint8_t in, uint8_t out) {
        defineInBufferSizes (in);
        defineOutBufferSizes(out);
    }

    void checkForRequestedData() {
        if (DATA_TO_REPORT.size() > 0 || DATA_TO_GET.size() > 0) {
            gpio_put(RQ_PIN, 1);
            return;
        }
        gpio_put(RQ_PIN, 0);
    }

    // true if valid false otherwise
    bool uartValidityCheck() {
        if (DATA_IN[1] == 0x05 || DATA_IN[1] == 0x04 || DATA_IN[1] == 0x06 || DATA_IN[1] == 0x07) {
            printf("validity ok\n");
            return true;
        }
        printf("Uart Header Incorrect\n");
        return false;
    }

    bool endOfMessageCorrect(uint8_t messSize) {

        if ((uint16_t)((DATA_IN[messSize-2] << 8) | DATA_IN[messSize-1]) != (uint16_t)crc16(DATA_IN, messSize-2)) {
            printf("Uart Check Incorrect\n");
            return false;
        }
        printf("crc ok\n");
        return true;
    }

    bool isMessForMe() {
        if (DATA_IN[0] != ADDRESS_ME) return false;
        return true;
    }

    bool isDataWrite() {
        if ((DATA_IN[2] & 0b1000'0000) != UART_DATA_WRITE) return false;
        return true;
    }

    bool checkWholeMess(uint8_t mess_size) {
        if (!uartValidityCheck() || !endOfMessageCorrect(mess_size)) {
            printf("bad message\n");
            uart_write_blocking(UART_PORT, MESS_ALL_BAD,  5);
            uart_reinit();
            uart_error_counter++;
            return false;
        }
        uart_error_counter = 0;
        printf("good mess\n");
        return true;
    }

    uint8_t pckgSizeToSend(uint8_t data_code) {
        uint8_t packg_size = 4;

        switch (data_code) {
            case DataType::SEND_ALL:
                for (int i = 0; i < DATA_TO_GET.size(); i++) {
                    packg_size += 2;
                }
                for (int i = 0; i < DATA_TO_REPORT.size(); i++) {
                    packg_size += DATA_REFS[DATA_TO_REPORT[i]].size+2;
                }
            break;
            default:
                packg_size += DATA_REFS[data_code].size+2;
            break;
        }

        printf("PACKED SIZE TO SEND DETERMINED TO BE: %i \n", packg_size);
        return packg_size;
    }



    uint8_t preparePackage(uint8_t data_code) {
        uint8_t pcg_size = pckgSizeToSend(data_code);
        defineOutBufferSizes(pcg_size);
        uint8_t data_counter = 2;

        switch (data_code) {
            case DataType::SEND_ALL:
                for (int i = 0; i < DATA_TO_GET.size(); i++) {
                    DATA_OUT[data_counter++] = DATA_REFS[DATA_TO_GET[i]].code | 0b1000'0000;
                    DATA_OUT[data_counter++] = 0;
                    //for (int z = 0; z < DATA_REFS[DATA_TO_GET[i]].size; z++) {
                    //    DATA_OUT[data_counter++] = DATA_REFS[DATA_TO_GET[i]].DATA_POINTER[z];
                    //}
                }
                for (int i = 0; i < DATA_TO_REPORT.size(); i++) {
                    DATA_OUT[data_counter++] = DATA_REFS[DATA_TO_REPORT[i]].code ;
                    DATA_OUT[data_counter++] = DATA_REFS[DATA_TO_REPORT[i]].size;
                    for (int z = 0; z < DATA_REFS[DATA_TO_REPORT[i]].size; z++) {
                        DATA_OUT[data_counter++] = DATA_REFS[DATA_TO_REPORT[i]].DATA_POINTER[z];
                    }
                }
            break;
            default:
                DATA_OUT[data_counter++] = DATA_REFS[data_code].code;
                DATA_OUT[data_counter++] = DATA_REFS[data_code].size;
                for (int z = 0; z < DATA_REFS[data_code].size; z++) {
                    DATA_OUT[data_counter++] = DATA_REFS[data_code].DATA_POINTER[z];
                }
            break;
        }
        
        DATA_OUT[0] = ADDRESS_ME;
        DATA_OUT[1] = 0x04;
        uint16_t crc = crc16(DATA_OUT, pcg_size-2);
        DATA_OUT[pcg_size-2] = crc >> 8;
        DATA_OUT[pcg_size-1] = crc;
        
        for (int i = 0; i < pcg_size; i++) {
            printf("p %i    =  %i \n", i, DATA_OUT[i]);
        }
        return pcg_size;
    }
    void writeData(uint8_t *data, uint8_t package_size) {
        uart_write_blocking(UART_PORT, data,  package_size);
        uart_default_tx_wait_blocking();
    }
    void writeData(uint8_t package_size) {
        uart_write_blocking(UART_PORT, DATA_OUT,  package_size);
        uart_default_tx_wait_blocking();
    }

    void readData(uint8_t data_code) {

    }

    void uart_fail_exit() {
        uart_reinit();
        clearUart();
    }

    void executeFunctionsToBeExecuted() {
        while (FUNCTIONS_TO_EXECUTE.size() > 0) {
            ((void(*)(void))DATA_REFS[FUNCTIONS_TO_EXECUTE[0]].funcToExecuteWhenNew)();
            FUNCTIONS_TO_EXECUTE.erase( FUNCTIONS_TO_EXECUTE.begin() + 0 );
            printf("\nDELETED FUNCTIONS_TO_EXECUTE i: %i\n", 0);
        }
    }


    void reciveMessage() {
        if (DATA_IN[1] == 0x04 && DATA_IN[2] != 0xff) {
            uint8_t pcg_size = DATA_IN[2];
            uint8_t header [5]= {ADDRESS_ME, 0x04, pcg_size};
            uint16_t crc = crc16(header, 3);
            header[3] = crc >> 8;
            header[4] = crc;
            writeData(header,  5);

            printf("START DATA RECIVED pcg size: %i\n",pcg_size);
            defineBufferSizes(pcg_size,pcg_size);
            uart_read_blocking(UART_PORT, DATA_IN,  pcg_size);
            
            if (!checkWholeMess(pcg_size)) return uart_fail_exit();
            else writeData(header,  5);

            printf("STARTING SEND\n");

            uint8_t counter = 2;

            while (counter < pcg_size-2) {
                uint8_t reg = DATA_IN[counter];
                uint8_t siz = DATA_IN[counter+1];
                counter += 2;
                
                if (!DATA_REFS[reg].DATA_POINTER) {
                    counter += siz;
                    if (DATA_REFS[reg].funcToExecuteWhenNew){
                        if (reg == 9) {
                            doReboot();
                        }
                        FUNCTIONS_TO_EXECUTE.push_back(reg);
                        // ((void(*)(void))DATA_REFS[reg].funcToExecuteWhenNew)();
                    }
                    continue;
                }
                printf("SIZE %i, REG %i\n", siz, reg);

                if (DATA_REFS[reg].writable) {
                    for (int z = 0; z < siz; z++) { 
                        DATA_REFS[reg].DATA_POINTER[z] = DATA_IN[counter++];
                        printf("%i\n", DATA_REFS[reg].DATA_POINTER[z]);
                    }
                }

                for (int z = siz; z < DATA_REFS[reg].size; z++) DATA_REFS[reg].DATA_POINTER[z] = 0x00;

                for (int i = 0; i < DATA_TO_GET.size(); i++){
                    if (DATA_TO_GET[i] == reg) {
                        DATA_TO_GET.erase( DATA_TO_GET.begin() + i );
                        printf("DELETED DATA_TO_GET i: %i", i);
                    }
                }

                if (DATA_REFS[reg].funcToExecuteWhenNew){
                    FUNCTIONS_TO_EXECUTE.push_back(reg);
                    // ((void(*)(void))DATA_REFS[reg].funcToExecuteWhenNew)();
                }                
            }
            checkForRequestedData();
            printf("DATA RECIVED\n");
        } else if (DATA_IN[1] == 0x04) {
            uint8_t pcg_size = preparePackage(DataType::SEND_ALL);
            uint8_t header [5]= {ADDRESS_ME, 0x04, pcg_size};
            
            uint16_t crc = crc16(header, 3);
            header[3] = crc >> 8;
            header[4] = crc;
            writeData(header,  5);
            writeData(DATA_OUT,  pcg_size);
            if (!uart_is_readable_within_us(UART_PORT, 5000)) {
                printf("cant read >:\n");
                return uart_fail_exit();
            };
            uart_read_blocking(UART_PORT, DATA_IN,  5);
            if (!checkWholeMess(5)) return uart_fail_exit();

            printf("woo4\n");
            
            if (DATA_IN[1] == 0x05) DATA_TO_REPORT.clear();
            
            checkForRequestedData();
            printf("messege ok\n");
        } else if (DATA_IN[1] == 0x05) {

        } else if (DATA_IN[1] == 0x06) {

        } else if (DATA_IN[1] == 0x07) {
            uint8_t pcg_size = DATA_IN[2];
            uint8_t header [5]= {ADDRESS_ME, 0x04, pcg_size};
            uint16_t crc = crc16(header, 3);
            header[3] = crc >> 8;
            header[4] = crc;
            writeData(header,  5);
            //184

            printf("START DATA RECIVED pcg size: %i\n",pcg_size);
            defineInBufferSizes(pcg_size);
            uart_read_blocking(UART_PORT, DATA_IN,  pcg_size);
            //194
            
            if (!checkWholeMess(pcg_size)) return uart_fail_exit();
            
            //203
            uint8_t mess_size = 0;

            for (int i = 2; i < pcg_size-2; i++) {
                mess_size += DATA_REFS[DATA_IN[i]].size+2;
            }

            mess_size += 4;
            defineOutBufferSizes(mess_size);
            header[2] = mess_size;
            crc = crc16(header, 3);
            header[3] = crc >> 8;
            header[4] = crc;
            writeData(header,  5);

            DATA_OUT[0] = ADDRESS_ME; DATA_OUT[1] = 0x04; 

            uint8_t counter = 2;
            for (int i = 2; i < pcg_size-2; i++) {
                DATA_OUT[counter++] = DATA_REFS[DATA_IN[i]].code;
                DATA_OUT[counter++] = DATA_REFS[DATA_IN[i]].size;
                for (int s = 0; s < DATA_REFS[DATA_IN[i]].size; s++) {
                    DATA_OUT[counter++] = DATA_REFS[DATA_IN[i]].DATA_POINTER[s];
                }
            }

            crc = crc16(DATA_OUT, mess_size);
            DATA_OUT[mess_size-2] = crc >> 8;
            DATA_OUT[mess_size-1] = crc;
            printf("before writing dataout %i\n", mess_size);
            writeData(DATA_OUT,  mess_size);
            if (!uart_is_readable_within_us(UART_PORT, 5000)) {
                printf("cant read >:\n");
                return uart_fail_exit();
            };
            uart_read_blocking(UART_PORT, DATA_IN,  5);
            if (!checkWholeMess(5)) return uart_fail_exit();

            
            printf("messege ok\n");
        }
    }

    void irqFunction() {
        uart_read_blocking(UART_PORT, DATA_IN,  5);
        for (int i = 0; i < 5; i++) {
            printf("\tmess raw %i \t| %i\n", DATA_IN[i], i);
        }
        if (!uartValidityCheck()) {
            uart_reinit();
        }
        if (!isMessForMe()) {
            printf("mess not for me\n");
            for (int i = 0; i < 5; i++) {
                printf("\tmess %i \t| %i\n", DATA_IN[i], i);
            }
            return uart_fail_exit();
        }

        if (!checkWholeMess(5)) return uart_fail_exit();
        printf("going to recive mess\n");
        reciveMessage();
    }

    void uart_reinit() {
        uart_deinit(UART_PORT);
        busy_wait_ms(1);
        initUart();
        initUartIRQ();
    }

    void initUartIRQ() {
        irq_set_exclusive_handler(UART_IRQ, irqFunction);
        irq_set_enabled(UART_IRQ, true);
        uart_set_irq_enables(UART_PORT, true, false);
    }

    void initUart() {
        printf("uart declaration\n");
        uart_init(UART_PORT, BOUND_RATE);
        gpio_set_function(TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(RX_PIN, GPIO_FUNC_UART);

        uart_set_fifo_enabled(UART_PORT, false);
        
        uart_set_hw_flow(UART_PORT, false, false);
        uart_set_format(UART_PORT, 8, 1, UART_PARITY_NONE);

        while (uart_is_readable(UART_PORT)) {
            uart_getc(UART_PORT);
        }

    }

    void init(bool initUartF, bool initIRQ, bool rqPin) {
        if (initUartF) initUart();
        if (initIRQ)   initUartIRQ();
        if (rqPin) {
            gpio_init(RQ_PIN);
            gpio_set_dir(RQ_PIN, GPIO_OUT);
            gpio_pull_down(RQ_PIN);
            gpio_put(RQ_PIN, 0);
        }
        uint16_t mess_pre_crc = crc16(MESS_ALL_OK, 3);
        MESS_ALL_OK[3] = mess_pre_crc >> 8;
        MESS_ALL_OK[4] = mess_pre_crc;
        mess_pre_crc = crc16(MESS_ALL_BAD, 3);
        MESS_ALL_BAD[3] = mess_pre_crc >> 8;
        MESS_ALL_BAD[4] = mess_pre_crc;


    }

}

#endif