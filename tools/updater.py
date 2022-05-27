#!/usr/bin/env python3

import asyncio
import requests
import uuid

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        s.connect(('1.1.1.1', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

async def initiate():
	url = "http://COFFEE.local/api/v1/update/initiate"

	request = {
		'URL'  : 'http://' + str(get_ip()) + ':8070/ESPresso.bin',
		'UUID' : str(uuid.uuid4())
	}

	x = requests.post(url, json = request, timeout = 0.01)
    
loop = asyncio.get_event_loop()
try:
    loop.run_until_complete(initiate())
finally:
    loop.close()
