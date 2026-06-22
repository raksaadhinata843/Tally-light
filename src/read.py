import serial
try:
    s = serial.Serial('COM3')
    print("Port is free!")
    s.close()
except serial.SerialException:
    print("Port is still busy! Close everything!")