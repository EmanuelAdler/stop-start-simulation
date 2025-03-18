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

#endif // SIMU_SENSORS_H