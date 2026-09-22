#include <pico/stdio.h>
#include <pico/time.h>

#include "usb.h"

// Watchdog Settings
#define WATCHDOG_PERIOD 2000  // If not updated for this duration, restart the program
#define WATCHDOG_DELAY 1000   // Duration before the program starts where nothing happens

void usb_with_watchdog_check_tasks();

void usb_with_watchdog_enable(USBConnection& usb_connection, uint8_t& read_reset);