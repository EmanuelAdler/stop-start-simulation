#include "powertrain_func.h"

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

/* CAN communication sockets*/
int sock_sender = -1;
int sock_receiver = -1;

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
        send_encrypted_message(sock_sender, "error_brake_not_pressed", CAN_ID_ERROR_DASH);
        log_toggle_event("Stop/Start: SWR2.8 (Brake not pressed or car is moving!)");
    }

    /* External and internal temperatures logic */

    if (internal_temp <= (temp_set + MAX_TEMP_DIFF) && external_temp <= temp_set)
    {
        cond2 = 1;
    }
    else
    {
        cond2 = 0;
        send_encrypted_message(sock_sender, "error_temperature_out_range", CAN_ID_ERROR_DASH);
        log_toggle_event("Stop/Start: SWR2.8 (Difference between internal and external temps out of range!)");
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
            send_encrypted_message(sock_sender, "error_engine_temperature_out_range", CAN_ID_ERROR_DASH);
            log_toggle_event("Stop/Start: SWR2.8 (Engine temperature out of range!)");
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
        send_encrypted_message(sock_sender, "error_battery_out_range", CAN_ID_ERROR_DASH);
        log_toggle_event("Stop/Start: SWR2.8 (Battery is not in operating range!)");
    }

    /* Door logic */

    if (!door_open)
    {
        cond5 = 1;
    }
    else
    {
        cond5 = 0;
        send_encrypted_message(sock_sender, "error_door_open", CAN_ID_ERROR_DASH);
        log_toggle_event("Stop/Start: SWR2.8 (One or more doors are opened!)");
    }

    /* Tilt angle logic */

    if (tilt_angle <= MAX_TILT_ANGLE)
    {
        cond6 = 1;
    }
    else
    {
        cond6 = 0;
        send_encrypted_message(sock_sender, "error_tilt_angle", CAN_ID_ERROR_DASH);
        log_toggle_event("Stop/Start: SWR2.8 (Tilt angle greater than 5 degrees!)");
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
            log_toggle_event("Stop/Start: Deactivated");
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
                send_encrypted_message(sock_sender, "RESTART", CAN_ID_ECU_RESTART);
                log_toggle_event("Stop/Start: Engine On");
                clock_gettime(CLOCK_MONOTONIC, restart_start);
                *is_restarting = true;
            }
            else
            {
                send_encrypted_message(sock_sender, "error_battery_low", CAN_ID_ERROR_DASH);
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
            send_encrypted_message(sock_sender, "ABORT", CAN_ID_ECU_RESTART);
            send_encrypted_message(sock_sender, "error_battery_drop", CAN_ID_ERROR_DASH);
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
            fflush(stdout);

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

        process_received_frame(sock_receiver);

        int unlock_result = pthread_mutex_unlock(&mutex_powertrain);
        if (unlock_result != 0)
        {
            return NULL;
        }

        sleep_microseconds(COMMS_TIME_US);
    }
    return NULL;
}