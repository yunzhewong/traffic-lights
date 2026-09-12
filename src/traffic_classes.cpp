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

PedestrianRequestButtons::PedestrianRequestButtons(uint north, uint east, uint south,
                                       uint west)
    : north(north), east(east), south(south), west(west) {}

    void PedestrianRequestButtons::add_callback(gpio_irq_callback_t callback) {
  north.add_callback(callback);
  east.add_callback(callback);
  south.add_callback(callback);
  west.add_callback(callback);
}

DirectionalLights::DirectionalLights(TrafficLight traffic, PedestrianLight ped1,
                                     PedestrianLight ped2)
    : traffic(traffic), ped1(ped1), ped2(ped2) {};
void DirectionalLights::set_red() {
  this->traffic.set_red();
  this->ped1.set_red();
  this->ped2.set_red();
}
void DirectionalLights::set_yellow() {
  this->traffic.set_yellow();
  this->ped1.set_red();
  this->ped2.set_red();
}
void DirectionalLights::set_off() {
  this->traffic.set_off();
  this->ped1.set_off();
  this->ped2.set_off();
}
void DirectionalLights::handle_pedestrian(bool ped1_request,
                                          bool ped2_request) {
  this->traffic.set_red();
  if (ped1_request) { // Turn on mid transition
    this->ped1.set_green();
  }
  if (ped2_request) { // Turn on mid transition
    this->ped2.set_green();
  }
}
void DirectionalLights::handle_green(bool flash_off, bool stored_ped1_request,
                                     bool stored_ped2_request) {
  this->traffic.set_green();
  if (stored_ped1_request && flash_off) { // Should not turn on mid transition
    this->ped1.set_off();
  } else {
    this->ped1.set_red();
  }
  if (stored_ped2_request && flash_off) { // Should not turn on mid transition
    this->ped2.set_off();
  } else {
    this->ped2.set_red();
  }
}
