#include "usb.h"

#include <bsp/board_api.h>
#include <pico/stdio.h>

#include "pico/bootrom.h"

static USBConnection* debug_channel = nullptr;

void debug_allocate(USBConnection* channel) {
    debug_channel = channel;
}

void debug_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    debug_channel->vprint(format, args);
    va_end(args);
}

USBConnection::USBConnection(int device_number) {
    this->m_device_number = device_number;
}

void USBConnection::vprint(const char* format, va_list args) {
    char print_buffer[128];
    int length = vsnprintf(print_buffer, sizeof(print_buffer), format, args);
    int write_length = MAX(this->writeable_bytes(), length);
    this->write((uint8_t*)print_buffer, length);
    this->flush();
}

bool USBConnection::is_connected() {
    return tud_cdc_n_connected(m_device_number);
};

uint32_t USBConnection::readable_bytes() {
    return tud_cdc_n_available(m_device_number);
}

// todo: remove when used for operations
uint32_t USBConnection::read(uint8_t* buffer, uint32_t max_size) {
    uint32_t count = tud_cdc_n_read(m_device_number, buffer, max_size);
    if (count == 1 && (char)buffer[0] == 'r') {
        reset_usb_boot(0, 0);
        return 1;
    }
    return count;
    // return tud_cdc_n_read(m_device_number, buffer, max_size);
}

uint32_t USBConnection::write(uint8_t* buffer, uint32_t size) {
    return tud_cdc_n_write(m_device_number, buffer, size);
}

void USBConnection::print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    this->vprint(format, args);
    va_end(args);
}

uint32_t USBConnection::writeable_bytes() {
    return tud_cdc_n_write_available(m_device_number);
}

void USBConnection::flush() {
    tud_cdc_n_write_flush(m_device_number);
}

void USBConnection::clear() {
    tud_cdc_n_write_clear(m_device_number);
}

void USBConnection::blocking_send(uint8_t* ptr, uint32_t length) {
    uint32_t written = 0;
    while (written < length) {
        uint32_t avail = this->writeable_bytes();
        if (avail == 0) {
            tud_task();  // service USB, free up TX buffer
            continue;
        }
        uint32_t chunk = MIN(avail, length - written);
        written += this->write(ptr + written, chunk);
    }
    this->flush();
}