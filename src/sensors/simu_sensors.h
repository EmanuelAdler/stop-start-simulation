#ifndef SIMU_SENSORS_H
#define SIMU_SENSORS_H

#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>

extern pthread_mutex_t mutex_sensors;

struct engine_temp{
    int engine_rpm;
    float vehicle_speed;
};

// Battery sensor parameters
#define DEFAULT_BATTERY_VOLTAGE     12.2F
#define DEFAULT_BATTERY_SOC         80.0F
#define BATTERY_SOC_INCREMENT       0.5F    // SOC increase when vehicle is moving
#define BATTERY_SOC_DECREMENT       0.2F    // SOC decrease when vehicle is stationary
#define MAX_BATTERY_SOC             100.0F  // Maximum SOC limit

// Function to update battery state based on vehicle speed
void update_battery_soc(float vehicle_speed);

// Battery sensor thread function
void* sensor_battery(void *arg);

#endif // SIMU_SENSORS_H