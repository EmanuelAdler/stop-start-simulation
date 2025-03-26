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

/* Temperatures operation */

#define MAX_TEMP_DIFF 5U

#define MAX_ENGINE_TEMP 105U
#define MIN_ENGINE_TEMP 70U

/* Start/Stop operation logic */

void check_conds(VehicleData *ptr_rec_data)
{

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
    double engi_temp = ptr_rec_data->engi_temp;

    // Activation conditions

    int cond1 = 0;
    int cond2 = 0;
    int cond3 = 0;
    int cond4 = 0;
    int cond5 = 0;
    int cond6 = 0;

    /* Speed, acceleration and brake logic */

    if (speed == 0 && !accel && brake)
    {
        cond1 = 1;
    }
    else
    {
        cond1 = 0;
        log_toggle_event("Stop/Start: Brake not pressed or car is moving!");
    }

    /* External and internal temperatures logic */

    if (internal_temp <= (temp_set + MAX_TEMP_DIFF) && external_temp <= temp_set)
    {
        cond2 = 1;
    }
    else
    {
        cond2 = 0;
        log_toggle_event("Stop/Start: Difference between internal and external temps out of range!");
    }

    /* Engine temperature logic */

    if (engi_temp >= MIN_ENGINE_TEMP && engi_temp <= MAX_ENGINE_TEMP)
    {
        cond3 = 1;
    }
    else
    {
        /* If start/stop is already active and engine temperature decreases,
        start stop won't be disabled by that */
        if (!start_stop_is_active)
        {
            cond3 = 0;
            log_toggle_event("Stop/Start: Engine temperature out of range!");
        }
    }

    /* Battery logic */

    if (batt_soc >= MIN_BATTERY_SOC || batt_volt > MIN_BATTERY_VOLTAGE)
    {
        cond4 = 1;
    }
    else
    {
        cond4 = 0;
        log_toggle_event("Stop/Start: Battery is not in operating range!");
    }

    /* Door logic */

    if (!door_open)
    {
        cond5 = 1;
    }
    else
    {
        cond5 = 0;
        log_toggle_event("Stop/Start: One or more doors are opened!");
    }

    /* Tilt angle logic */

    if (tilt_angle <= MAX_TILT_ANGLE)
    {
        cond6 = 1;
    }
    else
    {
        cond6 = 0;
        log_toggle_event("Stop/Start: Tilt angle greater than 5 degrees!");
    }

    /* Check start/stop */

    if (cond1 && cond2 && cond3 && cond4 && cond5 && cond6)
    {
        start_stop_is_active = true;
        log_toggle_event("Stop/Start: Activated");
    }
    else
    {
        start_stop_is_active = false;
        log_toggle_event("Stop/Start: deactivated!");
    }
}

void *function_start_stop(void *arg)
{
    VehicleData *ptr_rec_data = (VehicleData *)arg;

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_powertrain);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* Check the conditions to activate Stop/Start */

        check_conds(ptr_rec_data);

        printf("Start/Stop = %d\n", start_stop_is_active);

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
    init_logging_system();

    VehicleData rec_data = {0};

    pthread_mutex_init(&mutex_powertrain, NULL);

    pthread_t thread_start_stop;
    pthread_t thread_comms;

    pthread_create(&thread_start_stop, NULL, function_start_stop, &rec_data);
    pthread_create(&thread_comms, NULL, comms, NULL);

    pthread_join(thread_start_stop, NULL);
    pthread_join(thread_comms, NULL);

    pthread_mutex_destroy(&mutex_powertrain);

    cleanup_logging_system();

    return 0;
}