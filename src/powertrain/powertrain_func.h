#ifndef POWERTRAIN_H
#define POWERTRAIN_H

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
//#include <linux/time.h>

#include "can_comms.h"
#include "globals.h"

extern pthread_mutex_t mutex_powertrain;
extern int sock_sender;
extern int sock_receiver;
extern bool start_stop_is_active;

void check_conds(VehicleData *ptr_rec_data);
void handle_restart_logic(
    VehicleData *data,
    bool *is_restarting,
    struct timespec *restart_start);
void *function_start_stop(void *arg);
void *powertrain_comms(void *arg);
void sleep_microseconds_pw(long int msec);

#endif // POWERTRAIN_H