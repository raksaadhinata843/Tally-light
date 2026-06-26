import requests
import serial
import time
import xml.etree.ElementTree as ET

# Setup connection
ser = serial.Serial('COM4', 115200)

def get_vmix_tally():
    try:
        response = requests.get('http://192.168.1.100:58088/api/')
        root = ET.fromstring(response.content)
        return root.find('active').text, root.find('preview').text
    except:
        return None, None

while True:
    active, preview = get_vmix_tally()
    
    # Gunakan 0 jika tidak ada input aktif
    pgm_val = int(active) if (active and active.isdigit()) else 0
    pvw_val = int(preview) if (preview and preview.isdigit()) else 0
    
    # Kalkulasi mask
    pgm_mask = (1 << (pgm_val - 1)) if pgm_val > 0 else 0
    pvw_mask = (1 << (pvw_val - 1)) if pvw_val > 0 else 0
    
    print(f"vMix: PGM={pgm_val}({pgm_mask}), PVW={pvw_val}({pvw_mask})")
    
    # Kirim ke ESP32
    ser.write(bytes([pgm_mask, pvw_mask]))