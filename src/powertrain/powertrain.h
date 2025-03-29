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
//#include <linux/time.h>

#include "can_comms.h"

static pthread_mutex_t mutex_powertrain;
// extern int sock;

#endif // POWERTRAIN_H