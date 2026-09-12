#include "hardware/watchdog.h"
#include "usb.h"

// Watchdog Settings
#define WATCHDOG_PERIOD 2000  // If not updated for this duration, restart the program
#define WATCHDOG_DELAY  1000  // Duration before the program starts where nothing happens

void usb_with_watchdog_check_tasks() {
    tud_task();
    watchdog_update();
}

void usb_with_watchdog_enable(USBConnection& usb_connection, uint8_t& read_reset) {
    watchdog_enable(WATCHDOG_PERIOD, true);
    while (to_ms_since_boot(get_absolute_time()) < WATCHDOG_DELAY) {
        usb_with_watchdog_check_tasks();
        usb_connection.read(&read_reset, 1);
    }
}