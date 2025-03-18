#ifndef SIMU_SENSORS_H
#define SIMU_SENSORS_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>

extern pthread_mutex_t mutex_sensors;

#endif // SIMU_SENSORS_H