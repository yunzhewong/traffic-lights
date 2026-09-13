#include <bsp/board_api.h>
#include <hardware/timer.h>
#include <pico/stdio.h>

#include "math.h"
#include "usb.h"
#include "usb_with_watchdog.h"
#include "traffic_classes.h"

// GPIO Settings
// Pedestrian Requests
#define SOUTH_PEDESTRIAN_REQUEST 1
#define EAST_PEDESTRIAN_REQUEST 0
#define NORTH_PEDESTRIAN_REQUEST 27
#define WEST_PEDESTRIAN_REQUEST 28

// Traffic Lights
// North/South
#define NORTH_SOUTH_RED 3
#define NORTH_SOUTH_YELLOW 4
#define NORTH_SOUTH_GREEN 5 

// East/West
#define EAST_WEST_RED 26
#define EAST_WEST_YELLOW 22
#define EAST_WEST_GREEN 21

// Pedestrians
// North
#define NORTH_PEDESTRIAN_RED 20
#define NORTH_PEDESTRIAN_GREEN 19

// East
#define EAST_PEDESTRIAN_RED 8
#define EAST_PEDESTRIAN_GREEN 9

// South
#define SOUTH_PEDESTRIAN_RED 6
#define SOUTH_PEDESTRIAN_GREEN 7

// West
#define WEST_PEDESTRIAN_RED 18
#define WEST_PEDESTRIAN_GREEN 17

// Duration Constants
#define RED_DURATION 2
#define YELLOW_DURATION 2
#define GREEN_DURATION 5
#define PEDESTRIAN_GREEN_DURATION 2
#define PEDESTRIAN_CUTOFF 3
#define FLASH_PERIOD 0.5

USBConnection usb_connection = USBConnection(0);
USBConnection debug_connection = USBConnection(1);

struct pedestrian_request_t {
    volatile bool north = false;
    volatile bool east = false;
    volatile bool south = false;
    volatile bool west = false;
};

pedestrian_request_t pedestrian_request;

pedestrian_request_t copy_pedestrian_request(pedestrian_request_t current) {
    pedestrian_request_t output;
    output.north = current.north;
    output.east = current.east;
    output.south = current.south;
    output.west = current.west;
    return output;
}

enum TrafficState {
    RedBeforeNorthSouth,
    NorthSouthPedestrian,
    NorthSouthGreen,
    NorthSouthYellow,
    RedBeforeEastWest,
    EastWestPedestrian,
    EastWestGreen,
    EastWestYellow,
    Error
};

void handle_pedestrian_request(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio == NORTH_PEDESTRIAN_REQUEST) {
            pedestrian_request.north = true;
        } else if (gpio == EAST_PEDESTRIAN_REQUEST) {
            pedestrian_request.east = true;
        } else if (gpio == SOUTH_PEDESTRIAN_REQUEST) {
            pedestrian_request.south = true;
        } else if (gpio == WEST_PEDESTRIAN_REQUEST) {
            pedestrian_request.west = true;
        }
    }
}


double calc_time_since_transition_s(uint64_t current_time_us, uint64_t transition_time_us) {
    return (double)(current_time_us - transition_time_us) / 1e6;
}

bool flashing_toggle(uint64_t transition_time_us) {
    double time_since_transition_s = calc_time_since_transition_s(time_us_64(), transition_time_us);
    return ((int)(time_since_transition_s / FLASH_PERIOD) % 2 == 1);
}

bool flashing_toggle_with_cutoff(uint64_t transition_time_us, double cutoff_s) {
    double time_since_transition_s = calc_time_since_transition_s(time_us_64(), transition_time_us);
    if (time_since_transition_s > cutoff_s) {
        return false;
    }
    return flashing_toggle(transition_time_us);
}

void transition_after_duration(uint64_t& transition_time_us, uint32_t duration_s, TrafficState& state, TrafficState target_state, pedestrian_request_t& request_before_transition) {
    uint64_t current_time_us = time_us_64();
    double time_since_transition_s = calc_time_since_transition_s(current_time_us, transition_time_us);
    if (time_since_transition_s > duration_s) {
        state = target_state;
        transition_time_us = current_time_us;
        request_before_transition = copy_pedestrian_request(pedestrian_request);
    }
}

int main() {
    uint8_t read;
    usb_with_watchdog_enable(usb_connection, read);

    PedestrianRequestButtons requests = PedestrianRequestButtons(NORTH_PEDESTRIAN_REQUEST, EAST_PEDESTRIAN_REQUEST, SOUTH_PEDESTRIAN_REQUEST, WEST_PEDESTRIAN_REQUEST);
    requests.add_callback(&handle_pedestrian_request); 

    DirectionalLights north_south_direction = DirectionalLights(
        TrafficLight(NORTH_SOUTH_RED, NORTH_SOUTH_YELLOW, NORTH_SOUTH_GREEN),
        PedestrianLight(EAST_PEDESTRIAN_RED, EAST_PEDESTRIAN_GREEN),
        PedestrianLight(WEST_PEDESTRIAN_RED, WEST_PEDESTRIAN_GREEN)
    );
    DirectionalLights east_west_direction = DirectionalLights(
        TrafficLight(EAST_WEST_RED, EAST_WEST_YELLOW, EAST_WEST_GREEN), 
        PedestrianLight(NORTH_PEDESTRIAN_RED, NORTH_PEDESTRIAN_GREEN),
        PedestrianLight(SOUTH_PEDESTRIAN_RED, SOUTH_PEDESTRIAN_GREEN)
    );
    north_south_direction.set_off();
    east_west_direction.set_off();

    TrafficState state = TrafficState::RedBeforeNorthSouth;
    pedestrian_request_t request_before_transition = pedestrian_request;
    uint64_t transition_time_us = time_us_64();

    while (1) {
        usb_with_watchdog_check_tasks();
        usb_connection.read(&read, 1);

        switch (state) {
            case RedBeforeNorthSouth: {
                north_south_direction.set_red();
                east_west_direction.set_red();

                TrafficState target_state = NorthSouthGreen;
                if (pedestrian_request.east || pedestrian_request.west) {
                    target_state = NorthSouthPedestrian;
                }
                transition_after_duration(transition_time_us, RED_DURATION, state, target_state, request_before_transition);
                break;
            }
            case NorthSouthPedestrian: {
                north_south_direction.handle_pedestrian(pedestrian_request.east, pedestrian_request.west);
                east_west_direction.set_red();
                transition_after_duration(transition_time_us, PEDESTRIAN_GREEN_DURATION, state, TrafficState::NorthSouthGreen, request_before_transition);
                break;
            }
            case NorthSouthGreen: {
                pedestrian_request.east = false; // Find smarter way to do this
                pedestrian_request.west = false; // Find smarter way to do this
                bool flash_off = flashing_toggle_with_cutoff(transition_time_us, PEDESTRIAN_CUTOFF);
                north_south_direction.handle_green(flash_off, request_before_transition.east, request_before_transition.west);
                east_west_direction.set_red();
                transition_after_duration(transition_time_us, GREEN_DURATION, state, TrafficState::NorthSouthYellow, request_before_transition);
                break;
            }
            case NorthSouthYellow: {
                north_south_direction.set_yellow();
                east_west_direction.set_red();
                transition_after_duration(transition_time_us, YELLOW_DURATION, state, TrafficState::RedBeforeEastWest, request_before_transition);
                break;
            }
            case RedBeforeEastWest: {
                north_south_direction.set_red();
                east_west_direction.set_red();

                TrafficState target_state = EastWestGreen;
                if (pedestrian_request.north || pedestrian_request.south) {
                    target_state = EastWestPedestrian;
                }
                transition_after_duration(transition_time_us, RED_DURATION, state, target_state, request_before_transition);
                break;
            }
            case EastWestPedestrian: {
                north_south_direction.set_red();
                east_west_direction.handle_pedestrian(pedestrian_request.north, pedestrian_request.south);
                transition_after_duration(transition_time_us, PEDESTRIAN_GREEN_DURATION, state, TrafficState::EastWestGreen, request_before_transition);
                break;
            }
            case EastWestGreen: {
                pedestrian_request.north = false; // Find smarter way to do this
                pedestrian_request.south = false; // Find smarter way to do this
                bool flash_off = flashing_toggle_with_cutoff(transition_time_us, PEDESTRIAN_CUTOFF);
                north_south_direction.set_red();
                east_west_direction.handle_green(flash_off, request_before_transition.north, request_before_transition.south);
                transition_after_duration(transition_time_us, GREEN_DURATION, state, TrafficState::EastWestYellow, request_before_transition);
                break;
            }
            case EastWestYellow: {
                north_south_direction.set_red();
                east_west_direction.set_yellow();
                transition_after_duration(transition_time_us, YELLOW_DURATION, state, TrafficState::RedBeforeNorthSouth, request_before_transition);
                break;
            }
            case Error: {
                bool flash = flashing_toggle(transition_time_us);
                if (flash) {
                    north_south_direction.set_red();
                    east_west_direction.set_red();
                } else {
                    north_south_direction.set_off();
                    east_west_direction.set_off();
                }
                break;
            }
        }
        // To do some light combination validation after states are set
    }
}