#ifndef CAN_COMMS_H
#define CAN_COMMS_H

#include <stdbool.h>
#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include "globals.h"

#define CAN_INTERFACE ("vcan0")
#define CAN_DATA_LENGTH (8)
#define LOG_MESSAGE_SIZE (50)
#define SUCCESS_CODE (0)
#define ERROR_CODE (1)

extern bool start_stop_manual;
extern int sock;

// Vehicle simulation data
typedef struct {
    int time;
    double speed;
    int internal_temp;
    int external_temp;
    int door_open;
    double tilt_angle;
    int accel;
    int brake;
    int temp_set;
    double batt_soc;
    double batt_volt;
    double engi_temp;
    int     gear;
    int prev_brake;
    int prev_accel;
} VehicleData;

extern VehicleData rec_data;

bool check_is_valid_can_id_powertrain(canid_t can_id);

void process_received_frame_powertrain(int sock);

void parse_input_received_powertrain(char *input);

#endif //CAN_COMMS_H