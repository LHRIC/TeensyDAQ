import socket
import time
import serial

READ_SIZE = 128
METADATA_SIZE = 4
SOH = 0x01
EOT = 0x04
ACK = 0x06
NAK = 0x15
CAN = 0x18
START_FTP = 0x05

# Set up the serial connection
ser = serial.Serial('/dev/tty.usbmodem124749001', 115200)

ser.write(bytes([START_FTP]))
test = int(ord(ser.read()))
