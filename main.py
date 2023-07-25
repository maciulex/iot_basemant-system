import network
import socket
import time
from machine import WDT
from machine import Pin
import uasyncio as asyncio
import usocket
import secret
from machine import UART

wdt = WDT(timeout=8388)

ssid2 = secret.ssid2
password2 = secret.password2
ssid = secret.ssid
password = secret.password

packed_size_from_c = 50
#cs = machine.Pin(13, machine.Pin.OUT, machine.Pin.PULL_UP)
dateToSet      = machine.Pin(16, machine.Pin.IN, machine.Pin.PULL_DOWN)
getHarmonogram = machine.Pin(17, machine.Pin.IN, machine.Pin.PULL_DOWN)
signalToReflesh = machine.Pin(2, machine.Pin.OUT, value=0)
uartErrorCount = 0
 # Initialize SPI
uart = machine.UART(0,baudrate=9600,
                    tx = Pin(12),
                    rx = Pin(13),
                    bits = 8,
                    parity = None,
                    stop = 1,
                    flow = 0,
                    timeout = 50,
                    txbuf=packed_size_from_c,
                    rxbuf=packed_size_from_c)

#spi = machine.SPI(1,
#                baudrate=1000*1000,
#                polarity=1,
#                phase=1,
#                bits=8,
#                firstbit=machine.SPI.MSB,
#                sck=machine.Pin(14),
#                mosi=machine.Pin(15),
#                miso=machine.Pin(12))


wlan = network.WLAN(network.STA_IF)
f1Sig = False
f2Sig = False
async def getBoardStatus():
    while True:
        anwser = await getDataSpi()
        if (anwser[2] == 0xF5):
            break
        await asyncio.sleep(0.4)

    return anwser

def connect_to_network(tries = 0):
    wlan.active(True)
    wlan.config(pm = 0xa11140)  # Disable power-save mode
    if (tries == 0):
        wlan.connect(ssid, password)
    else:
        wlan.connect(ssid2, password2)
    max_wait = 10
    while max_wait > 0:
        if wlan.status() < 0 or wlan.status() >= 3:
            break
        max_wait -= 1
        print('waiting for connection...')
        time.sleep(1)

    if wlan.status() != 3:
        if (tries == 0):
            print("tring other network")
            connect_to_network(1)
            return
        else:
            raise RuntimeError('network connection failed')
    else:
        wdt.feed()
        print('connected')
        status = wlan.ifconfig()
        print('ip = ' + status[0])

async def serve_client(reader, writer):
    global cache, cache_counter, arr_size, f1Sig,f2Sig
    print("Client connected")
    request_line = await reader.readline()
    print("Request:", request_line)
    # We are not interested in HTTP request headers, skip them
    while await reader.readline() != b"\r\n":
        pass

    request = str(request_line)
    print(request)
    if (request.find("?resetHarmonogrameAndTime") != -1):
        wdt.feed()
        signalToReflesh.value(1)
        print("trying to reset harm and time")
    forced1 = request.find("?group1Forced")
    forced2 = request.find("?group2Forced")
    if (forced1 != -1 or forced2 != -1):
        if (forced1 != -1):
            forced1 = chr(request_line[forced1+12])
            if (forced1 =='1'):
                f1Sig = 1
            else:
                f1Sig = 0

        if (forced2 != -1):
            forced2 = chr(request_line[forced2+12])
            if (forced2 =='1'):
                f2Sig = 1
            else:
                f2Sig = 0
        
            
        print("force: ",forced1, " | ", forced2)
        await setForced(f1Sig, f2Sig)
    print(request)
    await asyncio.sleep(0.4)
    
    html = ""
    status = await getBoardStatus()
    html += "ctrlG1>"+str(status[3])+"/"
    html += "ctrlG2>"+str(status[4])+"/"
    html += "HarmG1>"+str(status[5])+"/"
    html += "HarmG2>"+str(status[6])+"/"
    html += "i1g1isControler>"+str(status[7])+"/"
    html += "i2g1allwaysON>"+str(status[8])+"/"
    html += "i3g2isControler>"+str(status[9])+"/"
    html += "i3g2allwaysON>"+str(status[10])+"/"
    html += "escoG1>"+str(status[11])+"/"
    html += "escoG2>"+str(status[12])+"/"
    html += "darkSens>"+str(status[13])+"/"
    html += "day>"+str(status[14])+"/"
    html += "month>"+str(status[15])+"/"
    html += "hour>"+str(status[16])+"/"
    html += "min>"+str(status[17])+"/"
    html += "sec>"+str(status[18])+"/"
    html += "year>"+str(status[19])+"/"
    html += "forced1>"+str(status[20])+"/"
    html += "forced2>"+str(status[21])+"/"
    
    html += "Voltage>"+str(((status[22] << 8) | status[23])/10)+"/"
    html += "Amps>"+str(((status[24] << 24) | (status[25] << 16) | (status[26] << 8) | (status[27]))*0.001)+"/"
    html += "Wats>"+str(((status[28] << 24) | (status[29] << 16) | (status[30] << 8) | (status[31]))/10)+"/"
    html += "WatsHours>"+str(((status[32] << 24) | (status[33] << 16) | (status[34] << 8) | (status[35])))+"/"
    html += "Frequency>"+str(((status[36] << 8) | (status[37]))/10)+"/"
    html += "PowerFactor>"+str(((status[38] << 8) | (status[39]))*0.01)+"/"
    lastStatus = "0"
    if (status[40] == 1):
        lastStatus = "1"
    html += "lastStatusPzem>"+lastStatus
    
    writer.write('HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n')
    writer.write(html)

    await writer.drain()
    await writer.wait_closed()

    print("Client disconnected")
async def setForced(group1 = False, group2 = False):
    toSends = [0] * packed_size_from_c
    toSends[0] = 0x03

    toSends[1] = group1
    toSends[2] = group2
    
    toSends = bytearray(toSends)
    while True:
        await getDataSpi(toSends)
        await asyncio.sleep(0.2)
        data = await getDataSpi(toSends)
        print(data[20], "| ",data[21], " | ", group1, " ", group2)
        if data[20] == group1 and group2 == data[21]:
            print("forced setted")
            return
        print("trying again to set forced")
        
    pass;

async def sendHarmonogram():
    print("request for harmonograme")
    socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
    request = "GET /getData/getHarmonogrameForBasement.php HTTP/1.1\r\nHost: 192.168.1.2\r\n\r\n"
    socketObject.connect(("192.168.1.2", 80))
    
    bytessent = socketObject.send(request)
    print("\r\nSent %d byte GET request to the web server." % bytessent)

    ss = socketObject.recv(512)
    ss = ss.decode('utf-8')
    socketObject.close()
    harmonograme = ss.split("<||>")
    harmonograme = harmonograme[1]
    harmonograme = harmonograme.split(";")
    counter = 1
    toSends = [0] * packed_size_from_c
    toSends[0] = 0x02
    for ele in harmonograme:
        b = ele.split("|")
        if (b[0] == ''):
            continue
        print("B: ",b)
        toSends[counter] = int(b[0])
        counter += 1
        
        tS = b[1].split(":")
        toSends[counter] = int(tS[0])
        counter += 1
        toSends[counter] = int(tS[1])
        
        tS = b[2].split(":")
        counter += 1
        toSends[counter] = int(tS[0])
        counter += 1
        toSends[counter] = int(tS[1])
        counter += 1
    toSends = bytearray(toSends)
    while True:
        await getDataSpi(toSends)
        await asyncio.sleep(0.2)
        if getHarmonogram.value() == 1:
            return
        anwser = await getDataSpi()
        wdt.feed()
        print("DOS2: ",anwser)
        if (anwser[0] == 0xFB):
            await asyncio.sleep(1);
            break

async def getTime():
    # Print what we received
    socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
    request = "GET /getData/getTime.php HTTP/1.1\r\nHost: 192.168.1.2\r\n\r\n"
    socketObject.connect(("192.168.1.2", 80))
    
    bytessent = socketObject.send(request)
    print("\r\nSent %d byte GET request to the web server." % bytessent)

    ss = socketObject.recv(512)
    ss = ss.decode('utf-8')
    socketObject.close()
    time = ss[-21: -1]
    time += ss[-1]
    time = time.split("-")
    
    # Close the socket's current connection now that we are finished.
    socketObject.close()
    for index, ele in enumerate(time):
        time[index] = int(ele)
    year = time[3]
    time[3] = year%100
    time.insert(3, int((year - year%100)/100))
    print(time)
    return time

async def sendTimeSpi():
    global wdt
    time = await getTime()
    time.insert(0, 0x01)
    wdt.feed()
    toSend = b"";
    
    for ele in (time):
        toSend += chr(ele)
    for i in range(packed_size_from_c- len(time)):
        toSend += chr(1)

    print(toSend)
    toSend = bytearray(toSend)
    while True:
        await getDataSpi(toSend)
        await asyncio.sleep(5)
        anwser = await getDataSpi()
        wdt.feed()
        print("DOS: ",anwser)
        if dateToSet.value() == 1:
            return
        if (anwser[0] == 0xFC):
            await asyncio.sleep(0.2);
            break

    print(time)
async def reInitUart():
    uart.deinit()
    uart = machine.UART(0,baudrate=9600,
                    tx = Pin(12),
                    rx = Pin(13),
                    bits = 8,
                    parity = None,
                    stop = 1,
                    flow = 0,
                    timeout = 50,
                    txbuf=packed_size_from_c,
                    rxbuf=packed_size_from_c)

async def getDataSpi(arg = None):
    global uartErrorCount
    data =   bytearray(packed_size_from_c)
    toSend = bytearray(([0xF1] * packed_size_from_c))
    print("gettingData")
    print("Data request")
    if arg != None:
        toSend = arg
    else:
        toSend = bytearray(toSend)
    toSend[48] = 0xA4
    toSend[49] = 0x5B
    print (''.join(' {:02x} '.format(x) for x in toSend))
    
    uart.write(toSend)
    while True:
        if (uart.txdone()):
            break
    await asyncio.sleep(.8);
    uart.readinto(data)
    await asyncio.sleep(1);
    await asyncio.sleep(1);
    if (data[48] + data[49] != 255):
        uartErrorCount += 1
        reInitUart()
        print("uart reinited")
        if (uartErrorCount > 5):
            print("UART to many errors board reset")
            machine.reset()
    print(bytearray(data))
    wdt.feed()
    print("data",data[0])
    
    return data
 

async def getData():
    while(True):
        await getDataSpi()
        await asyncio.sleep(30);
        

async def main():

    print('Connecting to Network...')
    connect_to_network()

    print('Setting up webserver...')
    asyncio.create_task(asyncio.start_server(serve_client, "0.0.0.0", 80))
    #asyncio.create_task(getData())
    while True:
        await asyncio.sleep(1);
        wdt.feed()
        if dateToSet.value() == 0:
            signalToReflesh.value(0)
            await sendTimeSpi()
            print("request for time")
        wdt.feed()
        if getHarmonogram.value() == 0:
            signalToReflesh.value(0)
            await sendHarmonogram()
            
        wdt.feed()
    
try:
    asyncio.run(main())
finally:
    asyncio.new_event_loop()



