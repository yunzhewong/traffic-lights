#pragma once

#include <cstdarg>
#include <cstdint>

#define USB_BUFFER_SIZE 256

class USBConnection {
   private:
    int m_device_number;

   public:
    USBConnection(int device_number);
    bool is_connected();
    uint32_t readable_bytes();
    uint32_t read(uint8_t* buffer, uint32_t max_size);
    uint32_t read_with_reset(uint8_t* buffer, uint32_t max_size);
    void print(const char* format, ...);
    void vprint(const char* format, va_list args);
    uint32_t write(uint8_t* buffer, uint32_t size);
    uint32_t writeable_bytes();
    void flush();
    void clear();
    void blocking_send(uint8_t* ptr, uint32_t length);
};

void debug_allocate(USBConnection* channel);
void debug_print(const char* format, ...);