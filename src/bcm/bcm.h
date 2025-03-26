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

#include "../common_includes/logging.h"

pthread_mutex_t mutex_bcm;

// Vehicle simulation data
typedef struct {
    int time;
    double speed;
    int internal_temp;
    int external_temp;
    int door_open;
    double tilt_angle;
    double engi_temp;
} VehicleData;

struct speed_data{

    int *simu_order;
    double *speed;

};

#endif // SIMU_BCM_H