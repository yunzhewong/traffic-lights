#include <bsp/board_api.h>
#include <pico/stdio.h>

#include "math.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"
#include "tusb_config.h"
#include "usb.h"
#include "usb_with_watchdog.cpp"


// GPIO Settings
// Pedestrian Requests
#define SOUTH_PEDESTRIAN_REQUEST 1
#define EAST_PEDESTRIAN_REQUEST 0
#define NORTH_PEDESTRIAN_REQUEST 27
#define WEST_PEDESTRIAN_REQUEST 28

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

volatile bool north_pedestrian_requested = false;
volatile bool east_pedestrian_requested = false;
volatile bool south_pedestrian_requested = false;
volatile bool west_pedestrian_requested = false;

void pedestrian_request(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio == NORTH_PEDESTRIAN_REQUEST) {
            north_pedestrian_requested = true;
        } else if (gpio == EAST_PEDESTRIAN_REQUEST) {
            east_pedestrian_requested = true;
        } else if (gpio == SOUTH_PEDESTRIAN_REQUEST) {
            south_pedestrian_requested = true;
        } else if (gpio == WEST_PEDESTRIAN_REQUEST) {
            west_pedestrian_requested = true;
        }
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

        void add_callback(gpio_irq_callback_t callback) {
            gpio_set_irq_enabled_with_callback(
                this->pin,
                GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                true,
                callback
            );
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
    uint8_t read;
    usb_with_watchdog_enable(usb_connection, read);

    InputGPIO north_pedestrian_request = InputGPIO(NORTH_PEDESTRIAN_REQUEST);
    north_pedestrian_request.add_callback(&pedestrian_request); 
    InputGPIO east_pedestrian_request = InputGPIO(EAST_PEDESTRIAN_REQUEST);
    east_pedestrian_request.add_callback(&pedestrian_request); 
    InputGPIO south_pedestrian_request = InputGPIO(SOUTH_PEDESTRIAN_REQUEST);
    south_pedestrian_request.add_callback(&pedestrian_request); 
    InputGPIO west_pedestrian_request = InputGPIO(WEST_PEDESTRIAN_REQUEST);
    west_pedestrian_request.add_callback(&pedestrian_request); 

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

    uint32_t count;
    while (1) {
        usb_with_watchdog_check_tasks();
        usb_connection.read(&read, 1);
        usb_connection.print("N: %d, E: %d, S: %d, W: %d\n", north_pedestrian_request.is_triggered(), east_pedestrian_request.is_triggered(), south_pedestrian_request.is_triggered(), west_pedestrian_request.is_triggered());
        if (count < 10) {
            north_south_traffic.set_green();
            east_pedestrian.set_green();
            west_pedestrian.set_green();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 20) {
            north_south_traffic.set_yellow();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 30) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 40) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_green();
            north_pedestrian.set_green();
            south_pedestrian.set_green();
        } else if (count < 50) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_yellow();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 60) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        }
        count = (count + 1) % 60;
        sleep_ms(100);
    }
}