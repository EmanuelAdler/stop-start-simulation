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
#include "../bcm/bcm_func.h"

typedef struct {
    bool start_stop_active; // 0 = off, 1 = on
} Actuators;

extern Actuators actuators;

void process_received_frame(int sock);
bool check_is_valid_can_id(canid_t can_id);
void print_dashboard_status();
void parse_input_received(char* input);
void process_engine_commands(char* input);
void process_errors(char* input);

#endif
