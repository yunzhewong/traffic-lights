#pragma once
#include <hardware/timer.h>
#include <pico/stdio.h>
#include "usb.h"
// DELIMITER
// LENGTH
// TYPE
// DATA 1
// DATA 2
// CRC

#define READ_TIMEOUT_US 1e6
#define DELIMITER 0xFF
#define COMMS_SIZE 6

uint8_t crc8(const uint8_t *data, size_t len);

uint8_t catch_message(USBConnection &usb_connection, uint8_t *read_buffer,
                      uint8_t &read_count, uint64_t &last_read_time);
