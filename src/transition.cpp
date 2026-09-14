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

void transition_state_t::transition_after_duration(uint32_t duration_s,
                                                   TrafficState target_state) {
  uint64_t current_time_us = time_us_64();
  double time_since_transition_s =
      calc_time_since_transition_s(current_time_us, transition_time_us);
  if (time_since_transition_s > duration_s) {
    this->state_enum = target_state;
    this->transition_time_us = current_time_us;
  }
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
    transition_state.transition_after_duration(GREEN_DURATION,
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
      } else {
        state_references.primary = this->north_south_direction;
        state_references.secondary = this->east_west_direction;
        state_references.ped1_requested = &this->pedestrian_request->east;
        state_references.ped2_requested = &this->pedestrian_request->west;
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
