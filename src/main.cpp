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
#define SOUTH_PEDESTRIAN_REQUEST 0
#define EAST_PEDESTRIAN_REQUEST 1
#define NORTH_PEDESTRIAN_REQUEST 28
#define WEST_PEDESTRIAN_REQUEST 27

// North/South
#define NORTH_SOUTH_RED 3
#define NORTH_SOUTH_YELLOW 4
#define NORTH_SOUTH_GREEN 5

// East/West
#define EAST_WEST_RED 26
#define EAST_WEST_YELLOW 22
#define EAST_WEST_GREEN 21

// Pedestrians
// North
#define NORTH_PEDESTRIAN_RED 20
#define NORTH_PEDESTRIAN_GREEN 19

// East
#define EAST_PEDESTRIAN_RED 8
#define EAST_PEDESTRIAN_GREEN 9

// South
#define SOUTH_PEDESTRIAN_RED 6
#define SOUTH_PEDESTRIAN_GREEN 7

// West
#define WEST_PEDESTRIAN_RED 18
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

class TrafficLight {
    public:
        TrafficLight(uint red_pin, uint yellow_pin, uint green_pin): red(red_pin), yellow(yellow_pin), green(green_pin) {
        }

        void set_on() {
            this->red.enable();
            this->yellow.enable();
            this->green.enable();
        }

        void set_red() {
            this->red.enable();
            this->yellow.disable();
            this->green.disable();
        }

        void set_yellow() {
            this->red.disable();
            this->yellow.enable();
            this->green.disable();
        }

        void set_green() {
            this->red.disable();
            this->yellow.disable();
            this->green.enable();
        }

        void set_off() {
            this->red.disable();
            this->yellow.disable();
            this->green.disable();
        }
    private:
        OutputGPIO red;
        OutputGPIO yellow;
        OutputGPIO green;
};

class PedestrianLight {
    public:
        PedestrianLight(uint red_pin, uint green_pin): red(red_pin), green(green_pin) {
        }

        void set_on() {
            this->red.enable();
            this->green.enable();
        }


        void set_red() {
            this->red.enable();
            this->green.disable();
        }

        void set_green() {
            this->red.disable();
            this->green.enable();
        }

        void set_off() {
            this->red.disable();
            this->green.disable();
        }
    private:
        OutputGPIO red;
        OutputGPIO green;
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

    TrafficLight north_south_traffic = TrafficLight(NORTH_SOUTH_RED, NORTH_SOUTH_YELLOW, NORTH_SOUTH_GREEN);
    TrafficLight east_west_traffic = TrafficLight(EAST_WEST_RED, EAST_WEST_YELLOW, EAST_WEST_GREEN);

    PedestrianLight north_pedestrian = PedestrianLight(NORTH_PEDESTRIAN_RED, NORTH_PEDESTRIAN_GREEN);
    PedestrianLight east_pedestrian = PedestrianLight(EAST_PEDESTRIAN_RED, EAST_PEDESTRIAN_GREEN);
    PedestrianLight south_pedestrian = PedestrianLight(SOUTH_PEDESTRIAN_RED, SOUTH_PEDESTRIAN_GREEN);
    PedestrianLight west_pedestrian = PedestrianLight(WEST_PEDESTRIAN_RED, WEST_PEDESTRIAN_GREEN);

    north_south_traffic.set_off();
    east_west_traffic.set_off();
    north_pedestrian.set_off();
    east_pedestrian.set_off();
    south_pedestrian.set_off();
    west_pedestrian.set_off();

    while (1) {
        check_tasks(read);
        usb_connection.print("N: %d, E: %d, S: %d, W: %d", north_pedestrian_request.is_triggered(), east_pedestrian_request.is_triggered(), south_pedestrian_request.is_triggered(), west_pedestrian_request.is_triggered());
        east_west_traffic.set_on();
        west_pedestrian.set_on();
        sleep_ms(250);
        east_west_traffic.set_off();
        west_pedestrian.set_off();
        sleep_ms(250);
    }
}