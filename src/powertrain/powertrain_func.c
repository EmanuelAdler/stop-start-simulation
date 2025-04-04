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

/* Condition check functions */
static bool check_movement_conditions(double speed, int accel, int brake, int gear)
{
    return (fabs(speed) == 0.0F) && (accel == 0) && (brake != 0) && (gear == 0);
}

static bool check_temperature_conditions(int internal_temp, int external_temp, int temp_set)
{
    return (internal_temp <= (temp_set + MAX_TEMP_DIFF)) && (external_temp >= temp_set);
}

static bool check_engine_temp_conditions(double engi_temp)
{
    return (engi_temp >= MIN_ENGINE_TEMP) && (engi_temp <= MAX_ENGINE_TEMP);
}

static bool check_battery_conditions(double batt_soc, double batt_volt)
{
    return (batt_soc >= MIN_BATTERY_SOC) && (batt_volt > MIN_BATTERY_VOLTAGE);
}

static bool check_door_conditions(int door_open)
{
    return (door_open == 0);
}

static bool check_tilt_conditions(double tilt_angle)
{
    return (tilt_angle <= MAX_TILT_ANGLE);
}

/* Condition evaluation with logging */

typedef const char* ErrorMessage;
typedef const char* LogMessage;

static int evaluate_condition_with_logging(bool condition, bool engine_off_state, 
    ErrorMessage error_msg, LogMessage log_msg)
{
    if (condition)
    {
        return 1;
    }
    
    if (!engine_off_state)
    {
        send_encrypted_message(sock_sender, error_msg, CAN_ID_ERROR_DASH);
        log_toggle_event(log_msg);
    }
    return 0;
}

/* Main function */
void check_disable_engine(VehicleData *ptr_rec_data)
{
    bool engine_off_local;
    int cond1;
    int cond2;
    int cond3;
    int cond4;
    int cond5;
    int cond6;
    
    engine_off_local = engine_off;

    /* Check each condition */
    cond1 = evaluate_condition_with_logging(
        check_movement_conditions(ptr_rec_data->speed, ptr_rec_data->accel, 
                                 ptr_rec_data->brake, ptr_rec_data->gear),
        engine_off_local,
        "error_brake_not_pressed",
        "Stop/Start: SWR2.8 (Brake not pressed or car is moving!)");
    
    cond2 = evaluate_condition_with_logging(
        check_temperature_conditions(ptr_rec_data->internal_temp,
                                   ptr_rec_data->external_temp,
                                   ptr_rec_data->temp_set),
        engine_off_local,
        "error_temperature_out_range",
        "Stop/Start: SWR2.8 (Difference between internal and external temps out of range!)");
    
    cond3 = evaluate_condition_with_logging(
        check_engine_temp_conditions(ptr_rec_data->engi_temp),
        engine_off_local,
        "error_engine_temperature_out_range",
        "Stop/Start: SWR2.8 (Engine temperature out of range!)");
    
    cond4 = evaluate_condition_with_logging(
        check_battery_conditions(ptr_rec_data->batt_soc, ptr_rec_data->batt_volt),
        engine_off_local,
        "error_battery_out_range",
        "Stop/Start: SWR2.8 (Battery is not in operating range!)");
    
    cond5 = evaluate_condition_with_logging(
        check_door_conditions(ptr_rec_data->door_open),
        engine_off_local,
        "error_door_open",
        "Stop/Start: SWR2.8 (One or more doors are opened!)");
    
    cond6 = evaluate_condition_with_logging(
        check_tilt_conditions(ptr_rec_data->tilt_angle),
        engine_off_local,
        "error_tilt_angle",
        "Stop/Start: SWR2.8 (Tilt angle greater than 5 degrees!)");

    /* Final decision */
    if ((cond1 != 0) && (cond2 != 0) && (cond3 != 0) && 
        (cond4 != 0) && (cond5 != 0) && (cond6 != 0))
    {
        if (engine_off_local == false)
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