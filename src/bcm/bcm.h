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

#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"

// Battery sensor parameters
#define DEFAULT_BATTERY_VOLTAGE     12.0F
#define DEFAULT_BATTERY_SOC         80.0F
#define BATTERY_SOC_INCREMENT       0.5F    // SOC increase when vehicle is moving
#define BATTERY_SOC_DECREMENT       0.2F    // SOC decrease when vehicle is stationary
#define MAX_BATTERY_SOC             100.0F  // Maximum SOC limit

pthread_mutex_t mutex_bcm;

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
} VehicleData;

// Battery sensor functions
void update_battery_soc(float vehicle_speed);
void* sensor_battery(void *arg);

#endif // SIMU_BCM_H