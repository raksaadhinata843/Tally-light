import requests
import serial
import time
import xml.etree.ElementTree as ET

# Setup connection to your Master ESP32 (adjust COM port)
ser = serial.Serial('COM4', 115200) 

def get_vmix_tally():
    try:
        response = requests.get('http://172.29.240.1:8088/api/')
        root = ET.fromstring(response.content)
        
        # Get active (Program) and preview cameras
        active = root.find('0').text
        preview = root.find('1').text
        return active, preview
    except:
        return None, None

last_active = None
while True:
    active, preview = get_vmix_tally()
    
    if active != last_active:
        command = f"TALLY:{active}:{preview}\n"
        ser.write(command.encode())
        last_active = active
        
    time.sleep(0.2) # Poll every 200ms