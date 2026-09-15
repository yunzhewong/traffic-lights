#pragma once

#include <pico/stdio.h>
#include <hardware/timer.h>
#include "traffic_classes.h"


// Duration Constants
#define RED_DURATION 2
#define YELLOW_DURATION 2
#define GREEN_DURATION 5
#define PEDESTRIAN_GREEN_DURATION 2
#define PEDESTRIAN_CUTOFF 3
#define FLASH_PERIOD 0.5
#define TICK_PERIOD 0.1


struct pedestrian_request_t {
    volatile bool north = false;
    volatile bool east = false;
    volatile bool south = false;
    volatile bool west = false;
};

enum TrafficState {
    Red,
    Pedestrian,
    Green,
    Yellow,
    Error
};

struct request_history_t {
    bool ped1;
    bool ped2;
};

double calc_time_since_transition_s(uint64_t current_time_us,
                                    uint64_t transition_time_us);

bool flashing_toggle(uint64_t transition_time_us);

bool flashing_toggle_with_cutoff(uint64_t transition_time_us, double cutoff_s);

struct transition_state_t{
    TrafficState state_enum;
    uint64_t transition_time_us;
    
    uint8_t current_ticks;
    uint8_t transition_ticks;

    void transition_after_duration(uint8_t duration_s,
                                   TrafficState target_state);
};

struct state_references_t {
    DirectionalLights* primary; 
    DirectionalLights* secondary; 
    volatile bool* ped1_requested; 
    volatile bool* ped2_requested; 
};

bool validate_lights(DirectionalLights &north_south,
                     DirectionalLights &east_west);

// NS Traffic (RYG)
// EW Traffic (RYG) ... 6 (in 1 byte)
// E Ped (RG)
// W Ped (RG) 
// N Ped (RG)
// S Ped (RG) ... 8 (in 1 byte)
// Pedestrian Request ... 4 (in 1 byte)
// Current Ticks ... 8 (in 1 byte)
// Transition Ticks ... 8 (in 1 byte)
struct packed_state_t {
    uint8_t traffic_byte;
    uint8_t pedestrian_byte;
    uint8_t request_byte;
    uint8_t current_ticks_byte;
    uint8_t transition_ticks_byte;
};

struct green_durations_t {
    uint8_t north_south;
    uint8_t east_west;
};

struct traffic_state_t {
  traffic_state_t(DirectionalLights *north_south_direction,
                  DirectionalLights *east_west_direction,
                  pedestrian_request_t *pedestrian_request);

  DirectionalLights *north_south_direction;
  DirectionalLights *east_west_direction;
  pedestrian_request_t *pedestrian_request;
  uint8_t *green_duration;
  green_durations_t green_durations;
  state_references_t state_references;
  transition_state_t transition_state;
  request_history_t request_before;

  void handle_transition();
  void change_green_durations(uint8_t north_south, uint8_t east_west);
  packed_state_t pack_state();

private:
  uint8_t pack_traffic();
  uint8_t pack_pedestrian();
  uint8_t pack_request();
};