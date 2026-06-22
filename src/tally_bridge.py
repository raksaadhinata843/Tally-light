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
    print(f"DEBUG: Active={active}, Preview={preview}")
    
    if active and preview:
        # Send Active Tally (State 1)
        if active == '3':
            ser.write(f"3:1\n".encode())
        elif preview == '3':
            ser.write(f"3:2\n".encode())
        else:
            ser.write(f"3:0\n".encode())
            
    time.sleep(2)