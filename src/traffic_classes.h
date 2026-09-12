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

class DirectionalLights {
    public:
      DirectionalLights(TrafficLight traffic, PedestrianLight ped1,
                        PedestrianLight ped2);
      void set_red();
      void set_yellow();
      void set_off();

      void handle_pedestrian(bool ped1_request, bool ped2_request);
      void handle_green(bool flash_off, bool stored_ped1_request,
                        bool stored_ped2_request);

      TrafficLight traffic;
      PedestrianLight ped1;
      PedestrianLight ped2;
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