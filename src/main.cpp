#include <bsp/board_api.h>
#include <hardware/timer.h>
#include <pico/stdio.h>

#include "math.h"
#include "usb.h"
#include "usb_with_watchdog.h"
#include "traffic_classes.h"
#include "transition.h"

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


USBConnection usb_connection = USBConnection(0);
USBConnection debug_connection = USBConnection(1);

pedestrian_request_t pedestrian_request;
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
    north_south_direction.set_red();
    DirectionalLights east_west_direction = DirectionalLights(
        TrafficLight(EAST_WEST_RED, EAST_WEST_YELLOW, EAST_WEST_GREEN), 
        PedestrianLight(NORTH_PEDESTRIAN_RED, NORTH_PEDESTRIAN_GREEN),
        PedestrianLight(SOUTH_PEDESTRIAN_RED, SOUTH_PEDESTRIAN_GREEN)
    );
    east_west_direction.set_red();

    traffic_state_t traffic_state = traffic_state_t { &north_south_direction, &east_west_direction, &pedestrian_request };

    while (1) {
        usb_with_watchdog_check_tasks();
        usb_connection.read_with_reset(&read, 1);
        traffic_state.handle_transition();
    }
}