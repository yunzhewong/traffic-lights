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

    void transition_after_duration(uint32_t duration_s,
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

struct traffic_state_t {
    traffic_state_t(DirectionalLights* north_south_direction, DirectionalLights* east_west_direction, pedestrian_request_t* pedestrian_request): north_south_direction(north_south_direction), east_west_direction(east_west_direction), pedestrian_request(pedestrian_request), state_references({north_south_direction, east_west_direction, &pedestrian_request->east, &pedestrian_request->west}), transition_state({ TrafficState::Red, time_us_64()}), request_before({*state_references.ped1_requested, *state_references.ped2_requested}) {

    }

    DirectionalLights *north_south_direction;
    DirectionalLights *east_west_direction;
    pedestrian_request_t* pedestrian_request;
    state_references_t state_references; 
    transition_state_t transition_state;
    request_history_t request_before;

    void handle_transition();
};