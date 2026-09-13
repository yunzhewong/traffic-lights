#pragma once
#include <bsp/board_api.h>
#include <pico/stdio.h>
#include "hardware/gpio.h"


class InputGPIO {
    public:
        InputGPIO(uint pin);
        bool is_triggered();
        void add_callback(gpio_irq_callback_t callback);

    private: 
        uint pin;
};

class OutputGPIO {
    public:
        OutputGPIO(uint pin);
        void set(bool value);
        void enable();
        void disable();
        bool is_on();
        bool is_off();

      private:
        uint pin;
        bool value;
};