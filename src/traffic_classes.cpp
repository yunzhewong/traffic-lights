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
bool TrafficLight::is_red() {
  return this->red.is_on() && this->yellow.is_off() && this->green.is_off();
}
bool TrafficLight::is_yellow() {
  return this->red.is_off() && this->yellow.is_on() && this->green.is_off();
}
bool TrafficLight::is_green() {
  return this->red.is_off() && this->yellow.is_off() && this->green.is_on();
}
bool TrafficLight::is_valid() { 
  // We don't consider the off state to be valid, as it should only be reached when in an error state.
  return is_red() || is_yellow() || is_green(); 
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
bool PedestrianLight::is_red() {
  return this->red.is_on() && this->green.is_off();
}
bool PedestrianLight::is_green() {
  return this->red.is_off() && this->green.is_on();
}
bool PedestrianLight::is_off() {
  return this->red.is_off() && this->green.is_off();
}

bool PedestrianLight::is_valid() { return is_red() || is_green() || is_off(); }

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
bool DirectionalLights::is_valid() {
  if (!traffic.is_valid() || !ped1.is_valid() || !ped2.is_valid()) {
    return false;
  }

  // invalid states:
  // 1: traffic yellow, either pedestrian is not red
  if (traffic.is_yellow() && (!ped1.is_red() || !ped2.is_red())) {
    return false;
  }

  // 2: traffic green, either pedestrians green
  if (traffic.is_green() && (ped1.is_green() || ped2.is_green())) {
    return false;
  }

  return true;
}
