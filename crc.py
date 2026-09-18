from dataclasses import dataclass
import time

import serial

@dataclass
class Times():
    north_south: int
    east_west: int

def crc8(data: bytes, length: int) -> int:
    crc = 0xFF
    for i in range(length):
        crc ^= data[i]
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc & 0xFF

def crcify(data: bytes):
    crc = crc8(data=data, length=len(data))
    return data + crc.to_bytes(1)

COMMS_TIMEOUT = 0.01
class USBCommunications():
    def __init__(self, port: str):
        self._socket = serial.Serial(port=port, timeout=COMMS_TIMEOUT)

    def read_state(self):
        write_data = bytes([0xFF, 0x06, 0x00, 0x00, 0x00])
        message = crcify(bytes(write_data))
        self._socket.write(message)
        response = self._socket.read(1024)
        if len(response) != 9:
            raise Exception(f"Invalid Response: {response}")
        return response[3:8]

    def write_times(self, times: Times) -> Times:
        write_data = bytes([0xFF, 0x06, 0x01, times.north_south, times.east_west])
        message = crcify(bytes(write_data))
        self._socket.write(message)
        response = self._socket.read(1024)
        if len(response) != 6:
            raise Exception(f"Invalid Response: {response}")
        return Times(north_south=response[3], east_west=response[4])

    def read_times(self) -> Times:
        write_data = bytes([0xFF, 0x06, 0x02, 0x00, 0x00])
        message = crcify(bytes(write_data))
        self._socket.write(message)
        response = self._socket.read(1024)
        if len(response) != 6:
            raise Exception(f"Invalid Response: {response}")
        return Times(north_south=response[3], east_west=response[4])
    
    def close(self):
        self._socket.close()

if __name__ == "__main__":
    comms = USBCommunications(port="/dev/ttyACM0")

    print(comms.read_state().hex())
    print(comms.read_times())
    comms.write_times(Times(north_south=1, east_west=10))
    time.sleep(1)
    try:
        while True:
            print(comms.read_state().hex())
            print(comms.read_times())
    except KeyboardInterrupt:
        pass
    comms.close()