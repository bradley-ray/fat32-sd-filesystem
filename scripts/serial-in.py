#!/usr/bin/env python3

import serial

com = serial.Serial('/dev/ttyACM0', baudrate=115200)

while(True):
    com.write(bytes(input("cmd: ") + '\n', 'ascii'))
