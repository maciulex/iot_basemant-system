import network
import socket
import time
from machine import Pin
import uasyncio as asyncio
import usocket
from machine import WDT
from machine import UART

pico_id = 0
pico_desc = "test_device"
pico_request_header = "?pico_id="+str(pico_id)+"&pico_desc="+pico_desc

wdt = WDT(timeout=8388)

requestForData = machine.Pin(2, machine.Pin.IN, machine.Pin.PULL_DOWN)

address_to = 0x22

async def bootAck(isPicoH = 0):
    await webRequest("/newSystem/traffic_control.php"+pico_request_header+"&action=bootAck")
    return [0,0,0];
async def getHarmonogram():
    harmonogram = await webRequest("/newSystem/traffic_control.php"+pico_request_header+"&action=getHarmonogram")
    harmonogram = harmonogram.split("||")
    if (harmonogram[0] == ''):
        return [];
    harmo_ready = [];
    for h in harmonogram:
        hh = h.split("|")
        for d in hh:
            harmo_ready.append(int(d))
    print(harmo_ready)
    return harmo_ready

async def getTime():
    time = await webRequest("/newSystem/traffic_control.php"+pico_request_header+"&action=getTime")
    print("/newSystem/traffic_control.php"+pico_request_header+"&action=getTime")
    time = time.split("-")
    timed = [];
    for t in time:
        n = int(t);
        if n < 255:
            timed.append(n)
        else:
            timed.append(int(t[0]+t[1]))
            timed.append(int(t[2]+t[3]))
    print(timed)
    return timed

from internet import *
import communication as comm

async def sendTime_h(address = address_to):
    wdt.feed()
    time = await getTime();
    wdt.feed()
    while (await comm.sendData(bytearray([7,len(time)])+bytearray(time),address)) is False:
        wdt.feed()
        await asyncio.sleep(0.1);
    wdt.feed()
async def sendHarmonogram_h(address = address_to):
    wdt.feed()
    harmo = await getHarmonogram();
    wdt.feed()
    while (await comm.sendData(bytearray([8,len(harmo)])+bytearray(harmo),address)) is False:
        wdt.feed()
        await asyncio.sleep(0.1);
    wdt.feed()

async def checkForRequests():
    while True:
        wdt.feed()
        if (requestForData.value()):
            #await comm.communication()
            data = False
            while data is False:
                wdt.feed()    
                data = await comm.getAllCommunicationRequest(address_to)
            wdt.feed()
            if len(data[0]):
                #report
                pass;
            if len(data[1]):
                for req in data[1]:
                    to_mach = req[0]
                    if to_mach == "TIME":
                        await sendTime_h(address_to)
                    elif to_mach == "HARMONOGRAM":
                        await sendHarmonogram_h(address_to);
                    wdt.feed()
                pass;

        wdt.feed()
        await asyncio.sleep(1);
    

async def main():

    print('Connecting to Network...')
    connect_to_network()


    print('Setting up webserver...')
    
    asyncio.create_task(asyncio.start_server(serve_client, "0.0.0.0", 80))
    wdt.feed()
    await asyncio.sleep(0.5);
    
    #asyncio.create_task(getData())
    #await bootAck()
    wdt.feed()
    asyncio.create_task(checkForRequests())
    print("init_done")
    #await comm.getAllCommunicationRequest(0x22)
    while True:
        await asyncio.sleep(2);
        #wdt.feed()
        #await dataCommunication()
        wdt.feed()


    
try:
    asyncio.run(main())
finally:
    asyncio.new_event_loop()