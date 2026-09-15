import serial

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

if __name__ == "__main__":
    device = serial.Serial(port="/dev/ttyACM0", timeout=0.01)

    write_data = bytes([0xFF, 0x06, 0x00, 0x00, 0x00])
    message = crcify(bytes(write_data))

    try:
        while True:
            device.write(message)
            response = device.read(1024)
            print(response.hex())
    except KeyboardInterrupt:
        pass
    device.close()