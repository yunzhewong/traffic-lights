#include "transition.h"

double calc_time_since_transition_s(uint64_t current_time_us,
                                    uint64_t transition_time_us) {
  return (double)(current_time_us - transition_time_us) / 1e6;
}
bool flashing_toggle(uint64_t transition_time_us) {
  double time_since_transition_s =
      calc_time_since_transition_s(time_us_64(), transition_time_us);
  return ((int)(time_since_transition_s / FLASH_PERIOD) % 2 == 1);
}
bool flashing_toggle_with_cutoff(uint64_t transition_time_us, double cutoff_s) {
  double time_since_transition_s =
      calc_time_since_transition_s(time_us_64(), transition_time_us);
  if (time_since_transition_s > cutoff_s) {
    return false;
  }
  return flashing_toggle(transition_time_us);
}
bool validate_lights(DirectionalLights &north_south,
                     DirectionalLights &east_west) {
  if (!north_south.is_valid() || !east_west.is_valid()) {
    return false;
  }

  // Pedestrian lights should be red if the traffic light is green
  if (north_south.traffic.is_green() &&
      (!east_west.ped1.is_red() || !east_west.ped2.is_red())) {
    return false;
  }
  if (east_west.traffic.is_green() &&
      (!north_south.ped1.is_red() || !north_south.ped2.is_red())) {
    return false;
  }

  // If one light is not red, the other one should be red.
  if (!north_south.traffic.is_red() && !east_west.traffic.is_red()) {
    return false;
  }
  return true;
}

void transition_state_t::transition_after_duration(uint8_t duration_s,
                                                   TrafficState target_state) {
  uint64_t current_time_us = time_us_64();
  double time_since_transition_s =
      calc_time_since_transition_s(current_time_us, transition_time_us);
  this->current_ticks = (uint8_t)(time_since_transition_s / TICK_PERIOD);
  this->transition_ticks = (uint8_t)((double)duration_s / TICK_PERIOD);
  if (time_since_transition_s > duration_s) {
    this->state_enum = target_state;
    this->transition_time_us = current_time_us;
  }
}
traffic_state_t::traffic_state_t(DirectionalLights *north_south_direction,
                                 DirectionalLights *east_west_direction,
                                 pedestrian_request_t *pedestrian_request)
    : north_south_direction(north_south_direction),
      east_west_direction(east_west_direction),
      pedestrian_request(pedestrian_request),
      state_references({north_south_direction, east_west_direction,
                        &pedestrian_request->east, &pedestrian_request->west}),
      transition_state({TrafficState::Red, time_us_64()}),
      request_before(
          {*state_references.ped1_requested, *state_references.ped2_requested}),
      green_durations({GREEN_DURATION, GREEN_DURATION}) {
  // because north south is the primary
  this->green_duration = &this->green_durations.north_south;
}
void traffic_state_t::handle_transition() {
  switch (transition_state.state_enum) {
  case Red: {
    TrafficState target_state = Green;
    if (*state_references.ped1_requested || *state_references.ped2_requested) {
      target_state = Pedestrian;
    }
    state_references.primary->set_red();
    state_references.secondary->set_red();
    transition_state.transition_after_duration(RED_DURATION, target_state);
    break;
  }
  case Pedestrian: {
    state_references.primary->traffic.set_red();
    if (*state_references.ped1_requested) {
      state_references.primary->ped1.set_green();
    }
    if (*state_references.ped2_requested) {
      state_references.primary->ped2.set_green();
    }
    state_references.secondary->set_red();
    transition_state.transition_after_duration(PEDESTRIAN_GREEN_DURATION,
                                               TrafficState::Green);

    if (transition_state.state_enum == TrafficState::Green) {
      request_before = {*state_references.ped1_requested,
                        *state_references.ped2_requested};
      *state_references.ped1_requested = false;
      *state_references.ped2_requested = false;
    }
    break;
  }
  case Green: {
    bool flash_off = flashing_toggle_with_cutoff(
        transition_state.transition_time_us, PEDESTRIAN_CUTOFF);
    state_references.primary->traffic.set_green();

    // If button is pressed when inside this state, it should be ignored but not
    // cleared It should still be queued for the next time.
    if (request_before.ped1 && flash_off) {
      state_references.primary->ped1.set_off();
    } else {
      state_references.primary->ped1.set_red();
    }
    if (request_before.ped2 && flash_off) {
      state_references.primary->ped2.set_off();
    } else {
      state_references.primary->ped2.set_red();
    }
    state_references.secondary->set_red();
    transition_state.transition_after_duration(*this->green_duration,
                                               TrafficState::Yellow);

    if (transition_state.state_enum == TrafficState::Yellow) {
      request_before.ped1 = false;
      request_before.ped2 = false;
    }
    break;
  }
  case Yellow: {
    state_references.primary->set_yellow();
    state_references.secondary->set_red();
    transition_state.transition_after_duration(YELLOW_DURATION,
                                               TrafficState::Red);

    if (transition_state.state_enum == TrafficState::Red) {
      if (state_references.primary == this->north_south_direction) {
        state_references.primary = this->east_west_direction;
        state_references.secondary = this->north_south_direction;
        state_references.ped1_requested = &this->pedestrian_request->north;
        state_references.ped2_requested = &this->pedestrian_request->south;
        this->green_duration = &green_durations.east_west;
      } else {
        state_references.primary = this->north_south_direction;
        state_references.secondary = this->east_west_direction;
        state_references.ped1_requested = &this->pedestrian_request->east;
        state_references.ped2_requested = &this->pedestrian_request->west;
        this->green_duration = &green_durations.north_south;
      }
    }
    break;
  }
  case Error: {
    bool flash = flashing_toggle(transition_state.transition_time_us);
    if (flash) {
      this->north_south_direction->set_red();
      this->state_references.secondary->set_red();
    } else {
      this->state_references.primary->set_off();
      this->state_references.secondary->set_off();
    }
    break;
  }
  }

  bool lights_valid =
      validate_lights(*this->north_south_direction, *this->east_west_direction);

  if (!lights_valid) {
    transition_state.state_enum = TrafficState::Error;
  }
}
void traffic_state_t::change_green_durations(uint8_t north_south,
                                             uint8_t east_west) {
  this->green_durations.north_south = north_south;
  this->green_durations.east_west = east_west;
}

packed_state_t traffic_state_t::pack_state() {
  uint8_t traffic_byte = this->pack_traffic();
  uint8_t pedestrian_byte = this->pack_pedestrian();
  uint8_t request_byte = this->pack_request();
  return packed_state_t{traffic_byte, pedestrian_byte, request_byte,
                        this->transition_state.current_ticks,
                        this->transition_state.transition_ticks};
}
uint8_t traffic_state_t::pack_traffic() {
  uint8_t north_south_contribution =
      north_south_direction->traffic.get_byte_state() << 4;
  uint8_t east_west_contribution =
      east_west_direction->traffic.get_byte_state();
  return north_south_contribution + east_west_contribution;
}
uint8_t traffic_state_t::pack_pedestrian() {
  uint8_t north_contribution = east_west_direction->ped1.get_byte_state() << 6;
  uint8_t east_contribution = north_south_direction->ped1.get_byte_state() << 4;
  uint8_t south_contribution = east_west_direction->ped2.get_byte_state() << 2;
  uint8_t west_contribution = north_south_direction->ped2.get_byte_state();
  return north_contribution + east_contribution + south_contribution +
         west_contribution;
}
uint8_t traffic_state_t::pack_request() {
  uint8_t output = 0;
  if (pedestrian_request->north) {
    output += 1 << 3;
  }
  if (pedestrian_request->east) {
    output += 1 << 2;
  }
  if (pedestrian_request->south) {
    output += 1 << 1;
  }
  if (pedestrian_request->west) {
    output += 1 << 0;
  }
  return output;
};
