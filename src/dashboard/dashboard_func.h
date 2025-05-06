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
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_VALUE_LENGTH 32
#define MAX_PENDING_FRAMES 64

// ncurses UI
#include "panels.h"

#define MSG_LOG_PANEL_OFFSET 42

static int num_deactivs = 0;
extern int sock_dash;
extern bool test_mode_dash;

typedef struct {
    struct can_frame frame;
    char decrypted[AES_BLOCK_SIZE];
} CanMessage;

// Thread communication structure
typedef struct {
    CanMessage messages[MAX_PENDING_FRAMES];
    uint8_t head;
    uint8_t tail;
    pthread_cond_t cond;
    sem_t sem;
    pthread_mutex_t mutex;
} CanBuffer;

typedef struct
{
    bool start_stop_active; // 0 = off, 1 = on
    int door_status;        // 0 = closed, 1 = open
    int error_system;       // 0 = no, 1 = yes
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

bool check_is_valid_can_id(canid_t can_id);
void parse_input_received(char *input);
void process_user_commands(char *input);
void process_engine_commands(char *input);
void process_sensor_readings(char *input);
void process_errors(char *input);
void sleep_microseconds(long int microseconds);

void init_can_buffer(void);
void cleanup_can_buffer(void);
void* can_receiver_thread(void* arg);
void* process_frame_thread(void* arg);

#endif
