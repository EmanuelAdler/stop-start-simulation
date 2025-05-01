#ifndef SIMU_BCM_H
#define SIMU_BCM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>
#include <semaphore.h>

#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"

extern sem_t sem_comms;

// CAN receiver
#define MAX_MSG_SIZE    50

// Battery sensor parameters
#define DEFAULT_BATTERY_VOLTAGE     12.0F
#define DEFAULT_BATTERY_SOC         80.0F
#define BATTERY_SOC_INCREMENT       0.04F    // Increase in battery SOC when vehicle is moving
#define BATTERY_SOC_DECREMENT       0.02F    // Decrease in battery SOC when vehicle is stationary
#define MAX_BATTERY_SOC             100.0F   // Maximum SOC limit

// Simulation orders and states
#define ORDER_STOP  0
#define ORDER_RUN   1
#define ORDER_PAUSE 2

#define STATE_STOPPED 0
#define STATE_RUNNING 1
#define STATE_PAUSED  2

// Gear definitions
#define PARKING 0
#define DRIVE   1

// AC sensor data
#define DEFAULT_SET_TEMP 23U

// Maximum number of simulation data points
#define SPEED_ARRAY_MAX_SIZE 5000

// Vehicle data structure
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
    int gear;
} VehicleData;

// Struct used in simu_speed_step
typedef struct {
    double **speed;
    int **accel;
    int **brake;
    int **gear;
} ControlData;

// Global variables (declared here as extern for use in main and testing)
extern pthread_mutex_t mutex_bcm;
extern volatile int simu_curr_step;
extern volatile int simu_state;
extern volatile int simu_order;
extern bool curr_gear;
extern float batt_volt;
extern float batt_soc;
extern int data_size;
extern VehicleData vehicle_data[SPEED_ARRAY_MAX_SIZE];
extern int sock_send;
extern int sock_recv;
extern char send_msg[];
extern bool test_mode;
extern bool fault_active;
extern int fault_start_time;
extern const int safety_timeout_ms;
extern bool data_updated;

// Function prototypes for simulation functions (for unit testing purposes)
void sleep_microseconds(long int microseconds);
int getCurrentTimeMs_real(void);
void read_csv_default(void);
void read_csv(const char *path);
void check_order(int order);
void simu_speed_step(VehicleData *sim_data, ControlData controls);
void* simu_speed(void *arg);
void send_data_update(void);
void check_health_signals(void);
void* comms(void *arg);
void *comms_reception(void *arg);
void update_battery_soc(double vehicle_speed);
void* sensor_battery(void *arg);
void check_system_disable(int sock_recv);

#endif // SIMU_BCM_H
