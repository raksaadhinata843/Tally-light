import requests
import serial
import time
import xml.etree.ElementTree as ET

# Setup connection
ser = serial.Serial('COM3', 115200)
time.sleep(2) # Wait for ESP32 to reset after opening port

def get_vmix_tally():
    try:
        response = requests.get('http://172.29.240.1:58088/api/', timeout=1)
        root = ET.fromstring(response.content)
        return root.find('active').text, root.find('preview').text
    except:
        return None, None

last_active = None
last_preview = None

while True:
    active, preview = get_vmix_tally()
    
    if active and preview:
        # Send Active Tally (State 1)
        if active != last_active:
            ser.write(f"{active}:1\n".encode())
            print(f"Sent Active: {active}")
            last_active = active
            
        # Send Preview Tally (State 2)
        if preview != last_preview:
            ser.write(f"{preview}:2\n".encode())
            print(f"Sent Preview: {preview}")
            last_preview = preview
            
    time.sleep(0.2)