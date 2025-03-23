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

extern pthread_mutex_t mutex_bcm;

/* Function that should receive an array of 2 elements (simulation order, pointer to current speed) */
static void *simu_speed(void *arg[]);

#endif // SIMU_BCM_H