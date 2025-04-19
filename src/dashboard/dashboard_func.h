#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ncurses UI
#include "panels.h"

#define MSG_LOG_PANEL_OFFSET    45
#define ERROR_LOG_SIZE          50

typedef struct {
    bool start_stop_active; // 0 = off, 1 = on
    int door_status; // 0 = closed, 1 = open
    int error_system; // 0 = no, 1 = yes
    double batt_soc;
    double batt_volt;
    double speed;
    int internal_temp;
    int external_temp;
    double tilt_angle;
    int accel; // 0 = not accelerating, 1 = accelerating
    int brake; // 0 = not braking, 1 = braking
    int temp_set;
    int gear; // 0 = P, 1 = D
    double engi_temp;
} Actuators;

extern Actuators actuators;

void process_received_frame(int sock);
bool check_is_valid_can_id(canid_t can_id);
void parse_input_received(char* input);
void process_user_commands(char* input);
void process_engine_commands(char* input);
void process_sensor_readings(char* input);
void process_errors(char* input);

#endif
