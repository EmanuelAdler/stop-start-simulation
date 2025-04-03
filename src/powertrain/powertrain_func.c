#include "powertrain_func.h"

#define SLEEP_TIME_US (100000U)
#define COMMS_TIME_US (50000U)
#define MICROSECS_IN_ONESEC (1000000L)
#define NANO_TO_MICRO (1000)

void sleep_microseconds_pw(long int msec)
{
    struct timespec tspec;
    tspec.tv_sec = msec / MICROSECS_IN_ONESEC;
    tspec.tv_nsec = (msec % MICROSECS_IN_ONESEC) * NANO_TO_MICRO;
    nanosleep(&tspec, NULL);
}

bool test_mode_powertrain = false;

/* Powertrain data */

VehicleData rec_data = {0};

bool engine_off = false;
pthread_mutex_t mutex_powertrain;

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

void check_disable_engine(VehicleData *ptr_rec_data)
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

    bool movement_cond = (speed == 0 && !accel && brake && !gear);

    if (movement_cond)
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

    bool ext_int_temp_cond = (internal_temp <= (temp_set + MAX_TEMP_DIFF) && external_temp >= temp_set);

    if (ext_int_temp_cond)
    {
        cond2 = 1;
    }
    else
    {
        /* If engine is already off and temperature is out of range,
        engine won't be turned on (cond2 won't be 0) */
        if (!engine_off)
        {
            cond2 = 0;
            send_encrypted_message(sock_sender, "error_temperature_out_range", CAN_ID_ERROR_DASH);
            log_toggle_event("Stop/Start: SWR2.8 (Difference between internal and external temps out of range!)");
        }
    }

    /* Engine temperature logic */

    bool engine_temp_cond = (engi_temp >= MIN_ENGINE_TEMP && engi_temp <= MAX_ENGINE_TEMP);

    if (engine_temp_cond)
    {
        cond3 = 1;
    }
    else
    {
        /* If engine is already off and engine temperature is out of range,
        engine won't be turned on (cond3 won't be 0) */
        if (!engine_off)
        {
            cond3 = 0;
            send_encrypted_message(sock_sender, "error_engine_temperature_out_range", CAN_ID_ERROR_DASH);
            log_toggle_event("Stop/Start: SWR2.8 (Engine temperature out of range!)");
        }
    }

    /* Battery logic */

    bool batt_cond = batt_soc >= MIN_BATTERY_SOC && batt_volt > MIN_BATTERY_VOLTAGE;

    if (batt_cond)
    {
        cond4 = 1;
    }
    else
    {
        /* If engine is already off and batery is operating out of range,
        engine won't be turned on (cond4 won't be 0) */
        if (!engine_off)
        {
            cond4 = 0;
            send_encrypted_message(sock_sender, "error_battery_out_range", CAN_ID_ERROR_DASH);
            log_toggle_event("Stop/Start: SWR2.8 (Battery is not in operating range!)");
        }
    }

    /* Door logic */

    if (!door_open)
    {
        cond5 = 1;
    }
    else
    {
        /* If engine is already off and at least one door is opened,
        engine won't be turned on (cond5 won't be 0) */
        if (!engine_off)
        {
        cond5 = 0;
        send_encrypted_message(sock_sender, "error_door_open", CAN_ID_ERROR_DASH);
        log_toggle_event("Stop/Start: SWR2.8 (One or more doors are opened!)");
        }
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
        /* Will only attempt to deactivate the engine if it's activated */
        if (!engine_off)
        {
            engine_off = true;
            log_toggle_event("Stop/Start: Engine turned Off");
        }
    }
}

void handle_engine_restart_logic(
    VehicleData *data,
    bool *engine_is_restarting,
    struct timespec *engine_restart_start)
{
    /* Restart trigger detection */
    if (engine_off && !(*engine_is_restarting))
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
                log_toggle_event("Stop/Start: Engine turned On");
                engine_off = false;
                clock_gettime(CLOCK_MONOTONIC, engine_restart_start);
                *engine_is_restarting = true;
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
    if (*engine_is_restarting)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        const long elapsed_us =
            ((now.tv_sec - engine_restart_start->tv_sec) * MICROSECS_IN_ONESEC) +
            ((now.tv_nsec - engine_restart_start->tv_nsec) / NANO_TO_MICRO);

        if (elapsed_us > MICROSECS_IN_ONESEC)
        {
            *engine_is_restarting = false; // Successful restart
        }
        else if (data->batt_volt < MIN_BATTERY_VOLTAGE)
        {
            send_encrypted_message(sock_sender, "ABORT", CAN_ID_ECU_RESTART);
            send_encrypted_message(sock_sender, "error_battery_drop", CAN_ID_ERROR_DASH);
            log_toggle_event("Fault: SWR3.4 (Battery Drop)");
            *engine_is_restarting = false;
        }
    }
}

void *function_start_stop(void *arg)
{
    VehicleData *ptr_rec_data = (VehicleData *)arg;
    static int prev_brake = 0;
    static int prev_accel = 0;
    static bool engine_is_restarting = false;
    static struct timespec engine_restart_start_time;

    while (!test_mode_powertrain)
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

            check_disable_engine(ptr_rec_data);

            printf("Start/Stop = %d\n", engine_off);
            fflush(stdout);

            handle_engine_restart_logic(
                ptr_rec_data,
                &engine_is_restarting,
                &engine_restart_start_time);
        }

        int unlock_result = pthread_mutex_unlock(&mutex_powertrain);
        if (unlock_result != 0)
        {
            return NULL;
        }

        sleep_microseconds_pw(SLEEP_TIME_US);
    }
    return NULL;
}

void *powertrain_comms(void *arg)
{

    (void)printf("Listening for CAN frames...\n");
    (void)fflush(stdout);

    while (!test_mode_powertrain)
    {
        int lock_result = pthread_mutex_lock(&mutex_powertrain);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* CAN Communication logic */

        process_received_frame_powertrain(sock_receiver);

        int unlock_result = pthread_mutex_unlock(&mutex_powertrain);
        if (unlock_result != 0)
        {
            return NULL;
        }

        sleep_microseconds_pw(COMMS_TIME_US);
    }
    return NULL;
}