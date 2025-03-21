#include "bcm.h"

#define SLEEP_TIME_US           (100000U)
#define MICROSECS_IN_ONESEC     (1000000L)
#define NANO_TO_MICRO           (1000)

void sleep_microseconds(long int msec) {
    struct timespec tspec;
    tspec.tv_sec = msec / MICROSECS_IN_ONESEC;
    tspec.tv_nsec = (msec % MICROSECS_IN_ONESEC) * NANO_TO_MICRO;
    nanosleep(&tspec, NULL);
}

/***** Simulation settings *****/

#define STATE_STOPPED   0
#define STATE_RUNNING   1
#define STATE_FINISHED  2

static int simu_state = STATE_STOPPED;

/***** Speed sensor data *****/



/* Current speed */
static float curr_speed = 0.0F;

/***** Accelerator pedal sensor data *****/

static bool is_accelerating = false;

/***** Brake sensor data *****/

static bool is_braking = false;

/***** Inclinometer sensor data *****/

/* Current vehicle angle - horizontal reference */
static float curr_vehicle_angle = 0.0F;


/***** Battery sensor data *****/

#define DEFAULT_BATTERY_VOLTAGE     12.0F
#define DEFAULT_BATTERY_SOC         80.0F

static float bat_voltage = DEFAULT_BATTERY_VOLTAGE;
static float bat_soc = DEFAULT_BATTERY_SOC;

/***** AC sensor data *****/

#define DEFAULT_OUTSIDE_TEMP       28.0F
#define DEFAULT_INSIDE_TEMP        25.0F
#define DEFAULT_SET_TEMP           23.0F

static float out_temp = DEFAULT_OUTSIDE_TEMP;
static float in_temp = DEFAULT_INSIDE_TEMP;
static float set_temp = DEFAULT_SET_TEMP;

/***** Door sensor data *****/

/* Any door opened */
static bool opened_door = false;

/***** Simulate speed *****/

static void *simu_sensor_gear_pos(void *arg) 
{
    int *rpm = (int *)arg;

    while (true) 
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0) 
        {
            return NULL;
        }

        if(simu_state == STATE_STOPPED){
            ;
        }


        int unlock_result = pthread_mutex_unlock(&mutex_bcm);
        if (unlock_result != 0) 
        {
            return NULL;
        }

        sleep_microseconds(SLEEP_TIME_US);
    }
    return NULL;
}