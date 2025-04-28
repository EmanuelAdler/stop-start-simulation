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

static bool restart_trigger = false;

extern pthread_mutex_t mutex_powertrain;
extern int sock_sender;
extern int sock_receiver;
extern bool engine_off;

void check_disable_engine(VehicleData *ptr_rec_data);
void handle_engine_restart_logic(
    VehicleData *data);
void *function_start_stop(void *arg);
void *powertrain_comms(void *arg);
void sleep_microseconds_pw(long int msec);

#endif // POWERTRAIN_H