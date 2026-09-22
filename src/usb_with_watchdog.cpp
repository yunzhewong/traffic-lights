#include "usb_with_watchdog.h"

#include "device/usbd.h"
#include "hardware/watchdog.h"

void usb_with_watchdog_check_tasks() {
    tud_task();
    watchdog_update();
}
void usb_with_watchdog_enable(USBConnection& usb_connection, uint8_t& read_reset) {
    stdio_init_all();
    tud_init(BOARD_TUD_RHPORT);

    watchdog_enable(WATCHDOG_PERIOD, true);
    while (to_ms_since_boot(get_absolute_time()) < WATCHDOG_DELAY) {
        usb_with_watchdog_check_tasks();
        usb_connection.read(&read_reset, 1);
    }
}