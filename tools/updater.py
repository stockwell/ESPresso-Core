#!/usr/bin/env python3

import asyncio
import requests
import uuid

async def initiate():
    url = "http://COFFEE.local/api/v1/update/initiate"

    request = {
            'URL'  : 'http://192.168.1.240:8070/ESPresso.bin',
            'UUID' : str(uuid.uuid4())
    }

    x = requests.post(url, json = request)
    
loop = asyncio.get_event_loop()
try:
    loop.run_until_complete(initiate())
finally:
    loop.close()
