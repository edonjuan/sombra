# sudo apt-get update
# sudo apt install python3-pip
# pip install pyserial

import time, serial

print ("Serialport_reader")
puertoSerie = serial.Serial('/dev/ttyUSB0', 9600)

while(True):
        try:
                msg = puertoSerie.readline()
                print(msg)
        except Exception as e:
                print(e)
        time.sleep(1)
