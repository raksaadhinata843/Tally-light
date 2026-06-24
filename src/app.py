import requests
import serial
import time
import xml.etree.ElementTree as ET

# Setup connection
ser = serial.Serial('COM3', 115200)

def get_vmix_tally():
    try:
        response = requests.get('http://192.168.0.5:58088/api/')
        root = ET.fromstring(response.content)
        return root.find('active').text, root.find('preview').text
    except:
        return None, None

while True:
    active, preview = get_vmix_tally()
    print(f"DEBUG: Active={active}, Preview={preview}")

    pgm = 0
    pvw = 0

    if active == '1':
        pgm = 1
    
    if preview == '1':
        pvw = 1

    ser.write(f"{pgm},{pvw}\n".encode())