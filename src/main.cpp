#include <bsp/board_api.h>
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

USBConnection usb_connection = USBConnection(0);
USBConnection debug_connection = USBConnection(1);

volatile bool north_pedestrian_requested = false;
volatile bool east_pedestrian_requested = false;
volatile bool south_pedestrian_requested = false;
volatile bool west_pedestrian_requested = false;

void pedestrian_request(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (gpio == NORTH_PEDESTRIAN_REQUEST) {
            north_pedestrian_requested = true;
        } else if (gpio == EAST_PEDESTRIAN_REQUEST) {
            east_pedestrian_requested = true;
        } else if (gpio == SOUTH_PEDESTRIAN_REQUEST) {
            south_pedestrian_requested = true;
        } else if (gpio == WEST_PEDESTRIAN_REQUEST) {
            west_pedestrian_requested = true;
        }
    }
}
int main() {
    uint8_t read;
    usb_with_watchdog_enable(usb_connection, read);

    PedestrianRequests requests = PedestrianRequests(NORTH_PEDESTRIAN_REQUEST, EAST_PEDESTRIAN_REQUEST, SOUTH_PEDESTRIAN_REQUEST, WEST_PEDESTRIAN_REQUEST);
    requests.add_callback(&pedestrian_request); 

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

    uint32_t count;
    while (1) {
        usb_with_watchdog_check_tasks();
        usb_connection.read(&read, 1);
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