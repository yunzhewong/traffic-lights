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

// DELIMITER
// LENGTH
// TYPE
// DATA 1
// DATA 2
// CRC

#define READ_TIMEOUT_US 1e6
#define DELIMITER 0xFF
#define COMMS_SIZE 6

uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;  // CRC-8/SMBUS polynomial
            else
                crc <<= 1;
        }
    }
    return crc;
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
    uint8_t read_buffer[COMMS_SIZE];
    uint8_t read_count;
    uint64_t last_read_time = time_us_64();

    while (1) {
        usb_with_watchdog_check_tasks();
        traffic_state.handle_transition();

        if (usb_connection.is_connected()) {
            uint8_t read_amount = usb_connection.read_with_reset(&read_buffer[read_count], COMMS_SIZE - read_count);
            read_count += read_amount;
            if (read_amount == 0) {
                // clear buffer if no read after timeout
                if (time_us_64() - last_read_time > READ_TIMEOUT_US) {
                    read_count = 0;
                }
                continue;
            }

            last_read_time = time_us_64();

            if (read_count > 0 && read_buffer[0] != DELIMITER) {
                // identify delimiter index
                uint8_t delimiter_index = 0;
                while (delimiter_index < read_count && read_buffer[delimiter_index] != DELIMITER) {
                    delimiter_index++;
                }
                // downshift
                for (int i = 0; i < read_count - delimiter_index; i++) {
                    read_buffer[i] = read_buffer[delimiter_index + i];
                }
                read_count -= delimiter_index;
            }

            if (read_count == 0) {
                // no delimiter, even after searching for it
                continue;
            }

            if (read_count < COMMS_SIZE) {
                // not enough data to consider
                continue;
            }

            debug_connection.print("%d-%d", read_count, read_amount);

            if (read_buffer[0] != DELIMITER || read_buffer[1] != COMMS_SIZE || read_buffer[COMMS_SIZE - 1] != crc8(read_buffer, COMMS_SIZE - 1)) {
                // did not match expected type
                // clear as buffer is already full
                read_count = 0;
                continue;
            }
            debug_connection.print("%d-%d", read_count, read_amount);

            uint8_t type = read_buffer[2];
            uint8_t data1 = read_buffer[3];
            uint8_t data2 = read_buffer[4];

            usb_connection.write(read_buffer, COMMS_SIZE);
            usb_connection.flush();
            read_count = 0;
        }

    }
}