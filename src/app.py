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

    if active:
        if active == '1':
            ser.write(f"1:1\n".encode())
        elif preview == '1':
            ser.write(f"1:2\n".encode())
        else:
            ser.write(f"1:0\n".encode())