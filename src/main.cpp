#include <bsp/board_api.h>
#include <hardware/timer.h>
#include <pico/stdio.h>

#include "math.h"
#include "pico/time.h"
#include "usb.h"
#include "usb_with_watchdog.cpp"
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
#define PEDESTRIAN_RED_FLASH_PERIOD 1

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
    EastWestRed,
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

bool pedestrian_light_on_when_traffic_green(uint64_t transition_time_us) {
    double time_since_transition_s = calc_time_since_transition_s(time_us_64(), transition_time_us);
    return ((int)(time_since_transition_s / PEDESTRIAN_RED_FLASH_PERIOD) % 2 == 1);
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

    TrafficLight north_south_traffic = TrafficLight(NORTH_SOUTH_RED, NORTH_SOUTH_YELLOW, NORTH_SOUTH_GREEN);
    TrafficLight east_west_traffic = TrafficLight(EAST_WEST_RED, EAST_WEST_YELLOW, EAST_WEST_GREEN);

    PedestrianLight north_pedestrian = PedestrianLight(NORTH_PEDESTRIAN_RED, NORTH_PEDESTRIAN_GREEN);
    PedestrianLight east_pedestrian = PedestrianLight(EAST_PEDESTRIAN_RED, EAST_PEDESTRIAN_GREEN);
    PedestrianLight south_pedestrian = PedestrianLight(SOUTH_PEDESTRIAN_RED, SOUTH_PEDESTRIAN_GREEN);
    PedestrianLight west_pedestrian = PedestrianLight(WEST_PEDESTRIAN_RED, WEST_PEDESTRIAN_GREEN);

    north_south_traffic.set_off();
    east_west_traffic.set_off();
    north_pedestrian.set_off();
    east_pedestrian.set_off();
    south_pedestrian.set_off();
    west_pedestrian.set_off();

    TrafficState state = TrafficState::RedBeforeNorthSouth;
    pedestrian_request_t request_before_transition = pedestrian_request;
    uint64_t transition_time_us = time_us_64();

    while (1) {
        usb_with_watchdog_check_tasks();
        usb_connection.read(&read, 1);

        switch (state) {
            case RedBeforeNorthSouth: {
                north_south_traffic.set_red();
                east_pedestrian.set_red();
                west_pedestrian.set_red();
                east_west_traffic.set_red();
                north_pedestrian.set_red();
                south_pedestrian.set_red();

                TrafficState target_state = NorthSouthGreen;
                if (pedestrian_request.east || pedestrian_request.east) {
                    target_state = NorthSouthPedestrian;
                }
                transition_after_duration(transition_time_us, RED_DURATION, state, target_state, request_before_transition);
                break;
            }
            case NorthSouthPedestrian: {
                north_south_traffic.set_red();
                if (pedestrian_request.east) { // Turn on mid transition 
                    east_pedestrian.set_green();
                }
                if (pedestrian_request.west) { // Turn on mid transition
                    west_pedestrian.set_green();
                }
                east_west_traffic.set_red();
                north_pedestrian.set_red();
                south_pedestrian.set_red();
                transition_after_duration(transition_time_us, PEDESTRIAN_GREEN_DURATION, state, TrafficState::NorthSouthGreen, request_before_transition);
                break;
            }
            case NorthSouthGreen: {
                north_south_traffic.set_green();
                bool ped_on = pedestrian_light_on_when_traffic_green(transition_time_us);
                if (request_before_transition.east && ped_on) { // Should not turn on mid transition
                    east_pedestrian.set_red();
                } else {
                    east_pedestrian.set_off();
                }
                if (request_before_transition.west && ped_on) { // Should not turn on mid transition
                    west_pedestrian.set_red();
                } else {
                    west_pedestrian.set_off();
                }
                east_west_traffic.set_red();
                north_pedestrian.set_red();
                south_pedestrian.set_red();
                transition_after_duration(transition_time_us, GREEN_DURATION, state, TrafficState::NorthSouthYellow, request_before_transition);
                break;
            }
            case NorthSouthYellow: {
                north_south_traffic.set_yellow();
                east_pedestrian.set_red();
                west_pedestrian.set_red();
                east_west_traffic.set_red();
                north_pedestrian.set_red();
                south_pedestrian.set_red();

                transition_after_duration(transition_time_us, YELLOW_DURATION, state, TrafficState::RedBeforeEastWest, request_before_transition);
                break;
            }
            case RedBeforeEastWest:
            case EastWestPedestrian:
            case EastWestGreen:
            case EastWestYellow:
            case EastWestRed:
                break;
            case Error:
                break;
            }
        if (count < 10) {
            north_south_traffic.set_green();
            east_pedestrian.set_green();
            west_pedestrian.set_green();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 20) {
            north_south_traffic.set_yellow();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 30) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 40) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_green();
            north_pedestrian.set_green();
            south_pedestrian.set_green();
        } else if (count < 50) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_yellow();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        } else if (count < 60) {
            north_south_traffic.set_red();
            east_pedestrian.set_red();
            west_pedestrian.set_red();
            east_west_traffic.set_red();
            north_pedestrian.set_red();
            south_pedestrian.set_red();
        }
        count = (count + 1) % 60;
        sleep_ms(100);
    }
}