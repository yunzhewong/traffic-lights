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
}

void OutputGPIO::set(bool value) {
    gpio_put(this->pin, value);
}

void OutputGPIO::enable() {
    gpio_put(this->pin, true);
}

void OutputGPIO::disable() {
    gpio_put(this->pin, false);
}
