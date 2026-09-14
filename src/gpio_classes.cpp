#include "gpio_classes.h"

InputGPIO::InputGPIO(uint pin) {
    this->pin = pin;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

bool InputGPIO::is_triggered() {
    return !gpio_get(this->pin);
}

void InputGPIO::add_callback(gpio_irq_callback_t callback) {
    gpio_set_irq_enabled_with_callback(
        this->pin,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
        true,
        callback
    );
}

OutputGPIO::OutputGPIO(uint pin) {
    this->pin = pin;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
    this->value = false;
}

void OutputGPIO::set(bool value) {
    gpio_put(this->pin, value);
    this->value = value;
}

void OutputGPIO::enable() {
    this->set(true);
}

void OutputGPIO::disable() { this->set(false); }
bool OutputGPIO::is_on() { return this->value; }
bool OutputGPIO::is_off() { return !this->value; };
