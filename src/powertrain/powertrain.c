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

VehicleData rec_data = {0};

static bool start_stop_is_active = false;

/* Battery operation */

#define MIN_BATTERY_VOLTAGE 12.2F
#define MIN_BATTERY_SOC 70.0F

/* Tilt angle operation */

#define MAX_TILT_ANGLE 5.0F

/* Temperatures operation */

#define MAX_TEMP_DIFF 5

#define MAX_ENGINE_TEMP 105
#define MIN_ENGINE_TEMP 70

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
    int gear = ptr_rec_data->gear;

    // Activation conditions

    int cond1 = 0;
    int cond2 = 0;
    int cond3 = 0;
    int cond4 = 0;
    int cond5 = 0;
    int cond6 = 0;

    /* Speed, gear, acceleration and brake logic */

    if (speed == 0 && !accel && brake && !gear)
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
        /* Will only activate stop/start once, when it's not activated */
        if (!start_stop_is_active)
        {
            start_stop_is_active = true;
            log_toggle_event("Stop/Start: Activated");
        }
    }
    else
    {
        /* Will only deactivate stop/start once, when it's activated */
        if (start_stop_is_active)
        {
            start_stop_is_active = false;
            log_toggle_event("Stop/Start: deactivated!");
        }
    }
}

static void handle_restart_logic(
    VehicleData *data,
    bool *is_restarting,
    struct timespec *restart_start)
{
    /* Restart trigger detection */
    if (start_stop_is_active && !(*is_restarting))
    {
        const bool brake_released = (data->prev_brake && !data->brake);
        const bool accelerator_pressed = (!data->prev_accel && data->accel);

        if (brake_released || accelerator_pressed)
        {
            /* Battery check */
            if (data->batt_volt >= MIN_BATTERY_VOLTAGE &&
                data->batt_soc >= MIN_BATTERY_SOC)
            {
                send_encrypted_message(sock, "RESTART", CAN_ID_ECU_RESTART);
                log_toggle_event("Engine On");
                clock_gettime(CLOCK_MONOTONIC, restart_start);
                *is_restarting = true;
            }
            else
            {
                log_toggle_event("Fault: SWR3.5 (Low Battery)");
            }
        }
    }

    /* Update previous states */
    data->prev_brake = data->brake;
    data->prev_accel = data->accel;

    /* Restart monitoring */
    if (*is_restarting)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        const long elapsed_us =
            ((now.tv_sec - restart_start->tv_sec) * MICROSECS_IN_ONESEC) +
            ((now.tv_nsec - restart_start->tv_nsec) / NANO_TO_MICRO);

        if (elapsed_us > MICROSECS_IN_ONESEC)
        {
            *is_restarting = false; // Successful restart
        }
        else if (data->batt_volt < MIN_BATTERY_VOLTAGE)
        {
            send_encrypted_message(sock, "ABORT", CAN_ID_ECU_RESTART);
            log_toggle_event("Fault: SWR3.4 (Battery Drop)");
            *is_restarting = false;
        }
    }
}

void *function_start_stop(void *arg)
{
    VehicleData *ptr_rec_data = (VehicleData *)arg;
    static int prev_brake = 0;
    static int prev_accel = 0;
    static bool is_restarting = false;
    static struct timespec restart_start_time;

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_powertrain);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* Only check Stop/Start if the driver have enabled it */

        if (start_stop_manual)
        {
            /* Check the conditions to activate Stop/Start */

            check_conds(ptr_rec_data);

            printf("Start/Stop = %d\n", start_stop_is_active);

            handle_restart_logic(
                ptr_rec_data,
                &is_restarting,
                &restart_start_time);
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

/* CAN Communication */

int sock = -1;

void *comms(void *arg)
{

    (void)printf("Listening for CAN frames...\n");
    (void)fflush(stdout);

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_powertrain);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* CAN Communication logic */

        process_received_frame(sock);

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
    if (!init_logging_system())
    {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    sock = create_can_socket(CAN_INTERFACE);
    if (sock < 0)
    {
        return ERROR_CODE;
    }

    pthread_mutex_init(&mutex_powertrain, NULL);

    pthread_t thread_start_stop;
    pthread_t thread_comms;

    pthread_create(&thread_start_stop, NULL, function_start_stop, &rec_data);
    pthread_create(&thread_comms, NULL, comms, NULL);

    pthread_join(thread_start_stop, NULL);
    pthread_join(thread_comms, NULL);

    pthread_mutex_destroy(&mutex_powertrain);

    close_can_socket(sock);

    cleanup_logging_system();

    return 0;
}