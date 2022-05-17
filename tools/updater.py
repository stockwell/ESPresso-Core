#!/usr/bin/env python3

import asyncio
import requests
import uuid

async def initiate():
    url = "http://192.168.1.166/api/v1/update/initiate"

    request = {
            'URL'  : 'http://some/fake/address/update.bin',
            'UUID' : str(uuid.uuid4())
    }

    x = requests.post(url, json = request)
    
loop = asyncio.get_event_loop()
try:
    loop.run_until_complete(initiate())
finally:
    loop.close()
