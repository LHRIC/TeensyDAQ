import socket
import time
import serial

# Define the host and port
HOST = "192.168.3.2"
PORT = 64922

READ_SIZE = 128
METADATA_SIZE = 6
SOH = 0x01
EOT = 0x04
ACK = 0x06
NAK = 0x15
CAN = 0x18
START_FTP = 0x05

"""
structure of packets:
[
HEADER
| HIGHER_ORDER_PACKET_ID | LOWER_ORDER_PACKET_ID
| HIGHER_ORDER_PACKET_ID_COMPLIMENT | LOWER_ORDER_PACKET_ID_COMPLIMENT
| DATA
| CHECKSUM
]

HEADER: 1 byte
HIGHER_ORDER_PACKET_ID: 1 byte
LOWER_ORDER_PACKET_ID: 1 byte
HIGHER_ORDER_PACKET_ID_COMPLIMENT: 1 byte
LOWER_ORDER_PACKET_ID_COMPLIMENT: 1 byte
DATA: 128 bytes
CHECKSUM: 1 byte
"""

# Set up the serial connection
ser = serial.Serial('/dev/tty.usbmodem124749001', 115200)


class Xmodem:
    def __init__(self, file):
        self.file = file
        self.packet_id = -1
        self.bytes_sent = 0

    def calculate_checksum(self, _data):
        return sum(_data) % 256

    def make_payload(self, _data, end=False):
        packet = bytearray()
        packet.append(EOT) if end else packet.append(SOH)

        # order split packet_id and compliment
        higher_order_packet_id = self.packet_id >> 8
        lower_order_packet_id = self.packet_id & 0xFF
        packet.append(higher_order_packet_id)
        packet.append(lower_order_packet_id)
        packet.append(higher_order_packet_id ^ 0xFF)
        packet.append(lower_order_packet_id ^ 0xFF)

        packet.extend(_data)
        packet.append(self.calculate_checksum(_data))

        return packet

    def harness(self):
        self.main()
        time.sleep(0.2)
        print("about to finish ftp")
        ser.reset_input_buffer()
        ser.reset_input_buffer()
        self.finish()

    def main(self):
        instruction = ACK

        with open(self.file, 'rb') as f:
            data = f.read(READ_SIZE)
            while data:
                # Wait for an instruction byte from the receiver
                if instruction == CAN:
                    print("CAN")
                    return -1
                elif instruction == ACK:
                    print("ACK")
                    self.packet_id += 1
                elif instruction == NAK:
                    print("NAK")
                    back = f.tell() - READ_SIZE
                    f.seek(f.tell() - READ_SIZE) if back >= 0 else f.seek(0)
                    continue
                else:
                    print("got garbage back " + str(instruction))
                    back = f.tell() - READ_SIZE
                    f.seek(f.tell() - READ_SIZE) if back >= 0 else f.seek(0)
                    continue

                # Check if data length is less than READ_SIZE (end of file)
                if len(data) < READ_SIZE:
                    self.bytes_sent += len(data)
                    data += b'\x00' * (READ_SIZE - len(data))
                    print("bytes sent at end", self.bytes_sent)
                    ser.write(self.make_payload(data))
                    self.packet_id += 1
                    break

                # Send payload
                if data:
                    print("data packet id ", self.packet_id)
                    self.bytes_sent += len(data)
                    print("bytes sent", self.bytes_sent)

                    ser.write(self.make_payload(data))

                # Read next data chunk
                data = f.read(READ_SIZE)

                # Read next instruction
                instruction = None
                while instruction is None:
                    instruction = int(ord(ser.read()))

    def finish(self):
        instruction = NAK
        while instruction == NAK:
            zeros = b'\x00' * READ_SIZE

            payload = self.make_payload(zeros, end=True)
            ser.write(payload)

            instruction = None
            while instruction is None:
                instruction = int(ord(ser.read()))

            if instruction == EOT:
                break


time.sleep(0.4)

# Send START_FTP as a byte
ser.write(bytes([START_FTP]))
start = int(ord(ser.read()))

if start == START_FTP:
    print("ftp started now")
    ftp = Xmodem('dataLarge.csv')
    ftp.harness()
else:
    print("wrong start code " + str(int(ord(start))))
