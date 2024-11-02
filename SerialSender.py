import serial
import signal, sys

ser = serial.Serial('/dev/ttyUSB0', baudrate=115200)

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    ser.close()
    sys.exit(0)


print(ser.name)
signal.signal(signal.SIGINT, signal_handler)
while(True):
    data = input("->")
    command = b""
    command += b'\x02'
    command += b'\xFA'

    data_bytes = bytes(data, 'utf-8')
    data_len = len(data_bytes)

    print("Len:", data_len)

    command += data_len.to_bytes(2, 'big')
    command += data_bytes
    command += b'\x03'
    print(command)
    ser.write(command)