#include "comms.h"

uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;  // CRC-8/SMBUS polynomial
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint8_t catch_message(USBConnection& usb_connection, uint8_t* read_buffer, uint8_t& read_count,
                      uint64_t& last_read_time) {
    if (!usb_connection.is_connected()) {
        return 0;
    }
    uint8_t read_amount = usb_connection.read_with_reset(&read_buffer[read_count], COMMS_SIZE - read_count);
    read_count += read_amount;
    if (read_amount == 0) {
        // clear buffer if no read after timeout
        if (time_us_64() - last_read_time > READ_TIMEOUT_US) {
            read_count = 0;
        }
        return 0;
    }

    last_read_time = time_us_64();

    if (read_count > 0 && read_buffer[0] != DELIMITER) {
        // identify delimiter index
        uint8_t delimiter_index = 0;
        while (delimiter_index < read_count && read_buffer[delimiter_index] != DELIMITER) {
            delimiter_index++;
        }
        // downshift
        for (int i = 0; i < read_count - delimiter_index; i++) {
            read_buffer[i] = read_buffer[delimiter_index + i];
        }
        read_count -= delimiter_index;
    }

    if (read_count == 0) {
        // no delimiter, even after searching for it
        return 0;
    }

    if (read_count < COMMS_SIZE) {
        // not enough data to consider
        return 0;
    }

    if (read_buffer[0] != DELIMITER || read_buffer[1] != COMMS_SIZE ||
        read_buffer[COMMS_SIZE - 1] != crc8(read_buffer, COMMS_SIZE - 1)) {
        // did not match expected type
        // clear as buffer is already full
        read_count = 0;
        return 0;
    }
    return read_count;
}