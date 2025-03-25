#include "powertrain.h"

#define SLEEP_TIME_US (100000U)
#define COMMS_TIME_US (50000U)
#define MICROSECS_IN_ONESEC (1000000L)
#define NANO_TO_MICRO (1000)

void sleep_microseconds(long int msec)
{
    struct timespec tspec;
    tspec.tv_sec = msec / MICROSECS_IN_ONESEC;
    tspec.tv_nsec = (msec % MICROSECS_IN_ONESEC) * NANO_TO_MICRO;
    nanosleep(&tspec, NULL);
}

/* Powertrain data */

static bool start_stop_is_active = false;

/* Battery operation */

#define MIN_BATTERY_VOLTAGE 12.2F
#define MIN_BATTERY_SOC 70.0F

/* Tilt angle operation */

#define MAX_TILT_ANGLE 5.0F

/* Start/Stop operation logic */

void *function_start_stop(void *arg)
{
    VehicleData *ptr_rec_data = (VehicleData *)arg;

    int time = ptr_rec_data->time;
    double speed = ptr_rec_data->speed;
    int internal_temp = ptr_rec_data->internal_temp;
    int external_temp = ptr_rec_data->external_temp;
    int door_open = ptr_rec_data->door_open;
    double tilt_angle = ptr_rec_data->tilt_angle;
    int accel = ptr_rec_data->accel;
    int brake = ptr_rec_data->brake;
    int temp_set = ptr_rec_data->temp_set;
    double batt_soc = ptr_rec_data->batt_soc;
    double batt_volt = ptr_rec_data->batt_volt;

    // Activation conditions

    int cond1 = 0;
    int cond2 = 0;
    int cond3 = 0;
    int cond4 = 0;
    int cond5 = 0;

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_powertrain);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* Speed, acceleration and brake logic */

        if (speed == 0 && !accel && brake)
        {
            cond1 = 1;
        }
        else
        {
            cond1 = 0;
            // implementar log
        }

        /* Temperatures logic */

        if (internal_temp <= (temp_set + 5) && external_temp <= temp_set)
        {
            cond2 = 1;
        }
        else
        {
            cond2 = 0;
            // implementar log
        }

        /* Battery logic */

        if (batt_soc >= MIN_BATTERY_SOC && batt_volt > MIN_BATTERY_VOLTAGE)
        {
            cond3 = 1;
        }
        else
        {
            cond3 = 0;
            // implementar log
        }

        /* Door logic */

        if (!door_open)
        {
            cond4 = 1;
        }
        else
        {
            cond4 = 0;
            // implementar log
        }

        /* Tilt angle logic */

        if (tilt_angle <= MAX_TILT_ANGLE)
        {
            cond5 = 1;
        }
        else
        {
            cond5 = 0;
            // implementar log
        }

        /* Check start/stop */

        if (cond1 && cond2 && cond3 && cond4)
        {
            start_stop_is_active = true;
        }
        else
        {
            start_stop_is_active = false;
            // implementar log
        }

        int unlock_result = pthread_mutex_unlock(&mutex_powertrain);
        if (unlock_result != 0)
        {
            return NULL;
        }

        sleep_microseconds(SLEEP_TIME_US);
    }
    return NULL;
}

void *comms(void *arg)
{

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_powertrain);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* Comms logic */

        // implementar comms

        int unlock_result = pthread_mutex_unlock(&mutex_powertrain);
        if (unlock_result != 0)
        {
            return NULL;
        }

        sleep_microseconds(COMMS_TIME_US);
    }
    return NULL;
}

int main()
{

    VehicleData rec_data = {0};

    pthread_mutex_init(&mutex_powertrain, NULL);

    pthread_t thread_start_stop, thread_comms;

    pthread_create(&thread_start_stop, NULL, function_start_stop, &rec_data);
    pthread_create(&thread_comms, NULL, comms, NULL);

    pthread_join(thread_start_stop, NULL);
    pthread_join(thread_comms, NULL);

    pthread_mutex_destroy(&mutex_powertrain);

    return 0;
}