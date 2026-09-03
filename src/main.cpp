#include <bsp/board_api.h>
#include <pico/stdio.h>

#include "hardware/watchdog.h"
#include "math.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"
#include "tusb_config.h"
#include "usb.h"

// Watchdog Settings
#define WATCHDOG_PERIOD 2000  // If not updated for this duration, restart the program
#define WATCHDOG_DELAY  1000  // Duration before the program starts where nothing happens

// GPIO Settings
// Pedestrian Requests
#define NORTH_PEDESTRIAN_REQUEST 0
#define EAST_PEDESTRIAN_REQUEST 1
#define SOUTH_PEDESTRIAN_REQUEST 2
#define WEST_PEDESTRIAN_REQUEST 3

// North/South
#define NORTH_SOUTH_RED 4
#define NORTH_SOUTH_YELLOW 5
#define NORTH_SOUTH_GREEN 6

// East/West
#define EAST_WEST_RED 7
#define EAST_WEST_YELLOW 8
#define EAST_WEST_GREEN 9

// Pedestrians
// North
#define NORTH_PEDESTRIAN_RED 10
#define NORTH_PEDESTRIAN_GREEN 11

// East
#define EAST_PEDESTRIAN_RED 12
#define EAST_PEDESTRIAN_GREEN 13

// South
#define SOUTH_PEDESTRIAN_RED 14
#define SOUTH_PEDESTRIAN_GREEN 15

// West
#define WEST_PEDESTRIAN_RED 16
#define WEST_PEDESTRIAN_GREEN 17

USBConnection usb_connection = USBConnection(0);
USBConnection debug_connection = USBConnection(1);

void check_tasks(uint8_t& read_reset) {
    tud_task();
    watchdog_update();
    usb_connection.read(&read_reset, 1);
}

void pre_start_blocking_watchdog_pause(uint8_t& read_reset) {
    while (to_ms_since_boot(get_absolute_time()) < WATCHDOG_DELAY) {
        check_tasks(read_reset);
    }
}

class InputGPIO {
    public:
        InputGPIO(uint pin) {
            this->pin = pin;
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_IN);
            gpio_pull_up(pin);
        }

        bool is_triggered() {
            return !gpio_get(this->pin);
        }

    private: 
        uint pin;
};

class OutputGPIO {
    public:
        OutputGPIO(uint pin) {
            this->pin = pin;
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_OUT);
            gpio_put(pin, false);
        }

        void set(bool value) {
            gpio_put(this->pin, value);
        }

        void enable() {
            gpio_put(this->pin, true);
        }

        void disable() {
            gpio_put(this->pin, false);
        }

    private:
        uint pin;
};

int main() {
    // ----------- SETUP --------------
    stdio_init_all();
    tud_init(BOARD_TUD_RHPORT);

    // ----------- WATCHDOG -----------
    // Add a watchdog that makes sure that memory crashes are fixable for a few seconds
    // The watchdog is a hardware timer that decrements until 0, restarting if 0 is reached.
    //
    watchdog_enable(WATCHDOG_PERIOD, true);
    uint8_t read;
    pre_start_blocking_watchdog_pause(read);

    InputGPIO north_pedestrian_request = InputGPIO(NORTH_PEDESTRIAN_REQUEST); 
    InputGPIO east_pedestrian_request = InputGPIO(EAST_PEDESTRIAN_REQUEST); 
    InputGPIO south_pedestrian_request = InputGPIO(SOUTH_PEDESTRIAN_REQUEST);
    InputGPIO west_pedestrian_request = InputGPIO(WEST_PEDESTRIAN_REQUEST);

    OutputGPIO north_south_red = OutputGPIO(NORTH_SOUTH_RED);
    OutputGPIO north_south_yellow = OutputGPIO(NORTH_SOUTH_YELLOW);
    OutputGPIO north_south_green = OutputGPIO(NORTH_SOUTH_GREEN);

    OutputGPIO east_west_red = OutputGPIO(EAST_WEST_RED);
    OutputGPIO east_west_yellow = OutputGPIO(EAST_WEST_YELLOW);
    OutputGPIO east_west_green = OutputGPIO(EAST_WEST_GREEN);

    OutputGPIO north_pedestrian_red = OutputGPIO(NORTH_PEDESTRIAN_RED);
    OutputGPIO north_pedestrian_green = OutputGPIO(NORTH_PEDESTRIAN_GREEN);

    OutputGPIO east_pedestrian_red = OutputGPIO(EAST_PEDESTRIAN_RED);
    OutputGPIO east_pedestrian_green = OutputGPIO(EAST_PEDESTRIAN_GREEN);

    OutputGPIO south_pedestrian_red = OutputGPIO(SOUTH_PEDESTRIAN_RED);
    OutputGPIO south_pedestrian_green = OutputGPIO(SOUTH_PEDESTRIAN_GREEN);

    OutputGPIO west_pedestrian_red = OutputGPIO(WEST_PEDESTRIAN_RED);
    OutputGPIO west_pedestrian_green = OutputGPIO(WEST_PEDESTRIAN_GREEN);

    while (1) {
        check_tasks(read);
        sleep_ms(100);
    }
}