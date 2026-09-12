#include <hardware/gpio.h>
#include <pico/types.h>
#include "gpio_classes.h"

class TrafficLight {
    public:
      TrafficLight(uint red_pin, uint yellow_pin, uint green_pin);
      void set_on();
      void set_red();
      void set_yellow();
      void set_green();
      void set_off();

    private:
      OutputGPIO red;
      OutputGPIO yellow;
      OutputGPIO green;
};


class PedestrianLight {
    public:
      PedestrianLight(uint red_pin, uint green_pin);
      void set_on();
      void set_red();
      void set_green();
      void set_off();

    private:
        OutputGPIO red;
        OutputGPIO green;
};

class PedestrianRequestButtons {
    public:
      PedestrianRequestButtons(uint north, uint east, uint south, uint west);
      void add_callback(gpio_irq_callback_t callback);

    private:
      InputGPIO north;
      InputGPIO east;
      InputGPIO south;
      InputGPIO west;
};