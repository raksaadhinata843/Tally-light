import requests
import serial
import time
import xml.etree.ElementTree as ET

# Setup connection to your Master ESP32 (adjust COM port)
ser = serial.Serial('COM3', 115200) 

def get_vmix_tally():
    try:
        response = requests.get('http://172.29.240.1:58088/api/', timeout=2)
        if response.status_code != 200:
            print(f"Failed to fetch vMix tally: HTTP {response.status_code}")
            return None, None
        root = ET.fromstring(response.content)
        
        # Get active (Program) and preview cameras
        active = root.find('active').text
        preview = root.find('preview').text
        return active, preview
    except Exception as e:
        print(f"Error fetching vMix tally: {e}")
        return None, None

last_active = None
while True:
    active, preview = get_vmix_tally()
    print(f"DEBUG: vMix says Active={active}, Preview={preview}")
    time.sleep(2)  # Poll every 200ms
    
    # Inside your while loop in tally_bridge.py:
    if active == '1': # Assume Input 1 is your camera
        current_state = 1 # 1 = Red (Active)
    elif preview == '1':
        current_state = 2 # 2 = Green (Preview)
    else:
        current_state = 0 # 0 = Off

    # Only send if state changed
    if current_state != last_active:
        msg = f"{current_state}\n" # Send JUST the number and a newline
        ser.write(msg.encode())
        last_active = current_state
        
    time.sleep(0.2) # Poll every 200ms