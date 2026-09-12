#include "traffic_classes.h"

TrafficLight::TrafficLight(uint red_pin, uint yellow_pin, uint green_pin)
    : red(red_pin), yellow(yellow_pin), green(green_pin) {}

void TrafficLight::set_on() {
  this->red.enable();
  this->yellow.enable();
  this->green.enable();
}

void TrafficLight::set_red() {
  this->red.enable();
  this->yellow.disable();
  this->green.disable();
}

void TrafficLight::set_yellow() {
  this->red.disable();
  this->yellow.enable();
  this->green.disable();
}

void TrafficLight::set_green() {
  this->red.disable();
  this->yellow.disable();
  this->green.enable();
}

void TrafficLight::set_off() {
  this->red.disable();
  this->yellow.disable();
  this->green.disable();
}

PedestrianLight::PedestrianLight(uint red_pin, uint green_pin)
    : red(red_pin), green(green_pin) {}

void PedestrianLight::set_on() {
  this->red.enable();
  this->green.enable();
}

void PedestrianLight::set_red() {
  this->red.enable();
  this->green.disable();
}

void PedestrianLight::set_green() {
  this->red.disable();
  this->green.enable();
}

void PedestrianLight::set_off() {
  this->red.disable();
  this->green.disable();
}

PedestrianRequests::PedestrianRequests(uint north, uint east, uint south,
                                       uint west)
    : north(north), east(east), south(south), west(west) {}

    void PedestrianRequests::add_callback(gpio_irq_callback_t callback) {
  north.add_callback(callback);
  east.add_callback(callback);
  south.add_callback(callback);
  west.add_callback(callback);
}
