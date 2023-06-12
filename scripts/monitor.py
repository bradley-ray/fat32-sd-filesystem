#!/usr/bin/env python3

import serial

com = serial.Serial('/dev/ttyACM0', baudrate=115200)

while response := com.readline():
    try:
        print(response.decode('ascii'), end='')
    except:
        print("error decoding")
