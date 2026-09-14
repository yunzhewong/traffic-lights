import serial


if __name__ == "__main__":
    device = serial.Serial(port="/dev/ttyACM0", timeout=1)
    device.write("r".encode())
    device.close()