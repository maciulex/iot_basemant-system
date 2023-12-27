import network
import socket
import time
from machine import Pin
import uasyncio as asyncio
import usocket
from machine import WDT
from machine import UART

from internet import *

import __main__



async def wdtfeed():
    __main__.wdt.feed()

async def crc16(data: bytes, length):
    table = [ 
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
        0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
        0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
        0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
        0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
        0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
        0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
        0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
        0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    ]
    
    crc = 0xFFFF
    for x in range(length):
        crc = (crc << 8) ^ table[(crc >> 8) ^ data[x]]
        crc &= 0xFFFF                                   # important, crc must stay 16bits all the way through
    return bytearray([(crc >> 8) & 0xff, crc & 0xff])

DATA = [
        [0, "NOTHING"    , 0],
        [1, "ERROR_CODE" , 5],
        [2, "STATUS"     , 1],
        [3, "BOOT"       , 0],
        [4, "OK"         , 0],
        [5, "BAD"        , 0],
        [6, "SEND_ALL"   , 0],
        [7, "TIME"       , 8],
        [8, "HARMONOGRAM", 60],
        [9, "REBOOT"     , 0],
        
        [10, "TEMPS_ONE_WIRE_E"                , 16],
        [11, "PZEM_E"                          , 25],
        [12, "DARK_SENSOR_E"                   , 1],
        [13, "ESCO_1_E"                        , 1],
        [14, "ESCO_2_E"                        , 1],
        [15, "RESET_ONE_WIRE_E"                , 0],
        [16, "RESET_PZEM_E"                    , 0],
        [17, "TEMP_TRESHOLD_FOR_HEAT_AND_OFF_E", 1],
        [18, "GROUP_1_STATUS_E"                , 1],
        [19, "GROUP_2_STATUS_E"                , 1],
        [20, "GROUP_3_STATUS_E"                , 1],
        [21, "GROUP_1_HARMONOGRAM_E"           , 1],
        [22, "GROUP_2_HARMONOGRAM_E"           , 1],
        [23, "GROUP_3_HARMONOGRAM_E"           , 1],
        [24, "DISABLE_GROUP_1_E"               , 1],
        [25, "DISABLE_GROUP_2_E"               , 1],
        [26, "DISABLE_GROUP_3_E"               , 1],
        [27, "FORCE_ENABLE_GROUP_1_E"          , 1],
        [28, "FORCE_ENABLE_GROUP_2_E"          , 1],
        [29, "FORCE_ENABLE_GROUP_3_E"          , 1],
        [30, "HEAT_AND_OFF_GROUP_1_E"          , 1],
        [31, "HEAT_AND_OFF_GROUP_2_E"          , 1],
        [32, "HEAT_AND_OFF_GROUP_3_E"          , 1],
        [33, "TEMP_TRESHOLD_CO_FOR_HEAT_AND_OFF_E", 1],
        [34, "SWITCH_1_STATE_E"                , 1],
        [35, "SWITCH_2_STATE_E"                , 1]
    ]

CommReqPin = Pin(2, Pin.IN, Pin.PULL_DOWN)


tx_pin = 0
rx_pin = 1
uart_id = 0
uart_baudrate = 9600

deafult_address = 0x22
uartErrorCount = 0

uart = UART(uart_id,baudrate=uart_baudrate,
                tx = Pin(tx_pin),
                rx = Pin(rx_pin),
                bits = 8,
                parity = None,
                stop = 1,
                flow = 0,
                timeout = 50,
                txbuf=150,
                rxbuf=150)

Request = [];
ADRESSES_ON_BUS = [];

async def reInitUart():
    global uart
    uart.deinit()
    uart = UART(uart_id,baudrate=uart_baudrate,
                    tx = Pin(tx_pin),
                    rx = Pin(rx_pin),
                    bits = 8,
                    parity = None,
                    stop = 1,
                    flow = 0,
                    timeout = 50,
                    txbuf=150,
                    rxbuf=150)

async def writeWithWait(data : bytearray):
    global uart;
    uart.write(data)
    while not (uart.txdone()):
        pass;

async def sendData(data, address):
    toSend = bytearray([address, 0x04])+data
    crc = await crc16(toSend, len(toSend))
    toSend = toSend+crc
    
    header = bytearray([address, 0x04, len(toSend)])
    crc = await crc16(header, len(header))
    header = header + crc
    await writeWithWait(header)
    d1 =  uart.read(5)
    if d1 is None or not checkCrc(d1) or (len(d1) != 5):
        print("BAD CRC1! send")
        return False
    await writeWithWait(toSend)
    d1 =  uart.read(5)
    if d1 is None or not checkCrc(d1) or (len(d1) != 5):
        print("BAD CRC2! send")
        return False
    return True

def checkCrc(data):
    d_crc = await crc16(data, len(data)-2)
    s_crc = (data[len(data)-2] << 8) | data[len(data)-1]
    if (d_crc == s_crc):
        return True;
    return False;

async def getAllCommunicationRequest(address):
    global uart
    dataToSend = [];
    report     = [];
    await wdtfeed()
    header = bytearray([address, 0x04, 0xff])
    crc = await crc16(header,3)
    header = header + crc
    
    await writeWithWait(header)
    
    d1 =  uart.read(5)
    if  d1 is None or not checkCrc(d1) or (len(d1) != 5):
        print("BAD CRC! all")
        return False
    d2 =  uart.read(d1[2])
    if d2 is None or not checkCrc(d2) or (len(d2) != d1[2]):
        print("BAD CRC2! all")
        return False
    
    header = bytearray([address, 0x05, d1[2]])
    crc = await crc16(header,3)
    header = header + crc
    await writeWithWait(header)
    await wdtfeed()
    print("a")
    x = 2
    while (x < d1[2]-2):
        item = d2[x] & 0b01111111
        what = d2[x] & 0b10000000
        val = [DATA[item][1], item, d2[x+1]];
        x += 2
        for z in range(val[2]):
            val.append(d2[x])
            x+=1
            
        if what:
            dataToSend.append(val)
        else:
            report.append(val)
    await wdtfeed()
    print(report)
    print(dataToSend)
    return [report, dataToSend]


async def getData(data, address):
    global uart
    
    header = bytearray([address, 0x07, len(data)+4])
    crc = await crc16(header, len(header))
    header = header + crc
    await writeWithWait(header)

    d1 =  uart.read(5)
    if d1 is None or not checkCrc(d1) or (len(d1) != 5):
        print("BAD CRC1!")
        return False

    await wdtfeed()
    header = bytearray([address, 0x07])+bytearray(data)
    crc = await crc16(header,len(header))
    header = header + crc
    
    await writeWithWait(header)
    
    d1 =  uart.read(5)
    if  d1 is None or not checkCrc(d1) or (len(d1) != 5):
        print("BAD CRC!")
        return False
    print("data size ", d1[2])
    d2 =  uart.read(d1[2])
    if d2 is None or not checkCrc(d2) or (len(d2) != d1[2]):
        print("BAD CRC2!")
        return False
    

    
    header = bytearray([address, 0x04, len(data)+4])
    crc = await crc16(header, len(header))
    header = header + crc
    await writeWithWait(header)

    print("result: ",d2)
    
    await wdtfeed()
    data = [];
    x = 2
    while (x < d1[2]-2):
        code = d2[x]
        size = d2[x+1]
        x += 2
        val = [code, size];
        for z in range(val[1]):
            val.append(d2[x])
            x+=1
        data.append(val)
        
    await wdtfeed()        
    return data;


reInitUart()