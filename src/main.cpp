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
#define NORTH_PEDESTRIAN_RED 12
#define NORTH_PEDESTRIAN_GREEN 13

// South
#define NORTH_PEDESTRIAN_RED 14
#define NORTH_PEDESTRIAN_GREEN 15

// West
#define NORTH_PEDESTRIAN_RED 16
#define NORTH_PEDESTRIAN_GREEN 17

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

    InputGPIO north_pedestrian_request = InputGPIO(HIGH_GPIO); 
    InputGPIO low_limit = InputGPIO(LOW_GPIO); 
    OutputGPIO step = OutputGPIO(STEP_GPIO); 
    OutputGPIO dir = OutputGPIO(DIR_GPIO); 

    bool move_positive = true;

    dir.enable();
    step.enable();


    while (1) {
        check_tasks(read);
        usb_connection.print("High: %d, Low: %d!\n", high_limit.is_triggered(), low_limit.is_triggered());
        
        bool hit_while_positive = move_positive && high_limit.is_triggered();
        bool hit_while_negative = !move_positive && low_limit.is_triggered(); 
        if (hit_while_negative || hit_while_positive) {
            move_positive = !move_positive;
        }
        dir.set(move_positive);
        step.disable();
        sleep_us(500);
        step.enable();
    }
}