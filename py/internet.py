import network
import socket
import time
from secret import *
from machine import Pin
import uasyncio as asyncio
import usocket
global wdt

import __main__
wlan = network.WLAN(network.STA_IF)

async def report(data):
    pass

async def webRequest(path, host = "192.168.1.2"):
    socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
    request = "GET "+path+" HTTP/1.1\r\nHost: "+host+"\r\n\r\n"
    socketObject.connect((host, 80))
    
    bytessent = socketObject.send(request)
    print("\r\nSent %d byte GET request to the web server." % bytessent)

    ss = socketObject.recv(512)
    ss = ss.decode('utf-8') 
    socketObject.close()
    if (ss.find("<||>") != -1):
        ss = ss.split("<||>")
        print(path, host)
        return ss[1]
    else:
        return True;
async def webRequest_POST(path, host = "192.168.1.2", payload = ""):
    payload += "\n\n"
    contentLength = "Content-Length: " + str( len( data ) ) + "\n\n"

    
    socketObject = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
    request = "POST "+path+" HTTP/1.1\r\nHost: "+host+"\r\n\r\n"
    request = request + contentLength + data
    socketObject.connect((host, 80))
    
    bytessent = socketObject.send(request)
    print("\r\nSent %d byte GET request to the web server." % bytessent)

    ss = socketObject.recv(512)
    ss = ss.decode('utf-8') 
    socketObject.close()
    if (ss.find("<||>") != -1):
        ss = ss.split("<||>")
        print(path, host)
        return ss[1]
    else:
        return True;

def connect_to_network(tries = 0):
    wlan.active(True)
    wlan.config(pm = 0xa11140)  # Disable power-save mode
    print("network: ",ssid[tries], ", ",password[tries])
    wlan.connect(ssid[tries], password[tries])
    max_wait = 10
    while max_wait > 0:
        if wlan.status() < 0 or wlan.status() >= 3:
            break
        max_wait -= 1
        print('waiting for connection...')
        time.sleep(1)
    
    
    if wlan.status() != 3:
        tries += 1
        if (tries < len(ssid)):
            print("tring other network ", tries)
            connect_to_network(tries)
            return
        else:
            raise RuntimeError('network connection failed')
    else:
        __main__.wdt.feed()
        print('connected')
        status = wlan.ifconfig()
        print('ip = ' + status[0])

async def serve_client(reader, writer):
    print("Client connected")
    request_line = await reader.readline()
    print("Request:", request_line)
    # We are not interested in HTTP request headers, skip them
    while await reader.readline() != b"\r\n":
        pass

    request = str(request_line)
    print(request)
    #request.find("?ddd")
    html = ""
    if request.find("?action=sendRegisters") != -1:
        registers = request.split("%3C||%3E")[1]
        registers = registers.split("||")
        regi_ready = [];
        for r in registers:
            rr = r.split("|")
            counter = 1
            for d in rr:
                if counter == 2:
                    regi_ready.append(len(rr)-1)
                
                regi_ready.append(int(d))
                counter += 1
        print(regi_ready)
        __main__.wdt.feed()
        await __main__.comm.sendData(bytearray(regi_ready), 0x22)
        __main__.wdt.feed()
    if request.find("?action=getRegisters") != -1:
        registers = request.split("%3C||%3E")[1]
        registers = registers.split("|")
        regi_ready = [];
        for r in registers:
            regi_ready.append(int(r))

        print(regi_ready)
        __main__.wdt.feed()
        print("registers: ", regi_ready)
        res = await __main__.comm.getData(regi_ready, 0x22)
        for r in res:
            for d in r:
                html += str(d)+"|"
            html += "||"
        __main__.wdt.feed()
    print(request)
    


    
    writer.write('HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n')
    writer.write(html)
    
    await writer.drain()
    await writer.wait_closed()
    __main__.wdt.feed()
    print("Client disconnected")