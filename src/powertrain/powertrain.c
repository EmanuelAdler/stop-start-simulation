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

static bool start_stop_manual = false;

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
        start_stop_is_active = true;
        log_toggle_event("Stop/Start: Activated");
    }
    else
    {
        start_stop_is_active = false;
        log_toggle_event("Stop/Start: deactivated!");
    }
}

static void handle_restart_logic(
    VehicleData* data,
    bool* is_restarting,
    struct timespec* restart_start
) {
    /* Restart trigger detection */
    if (start_stop_is_active && !(*is_restarting)) {
        const bool brake_released = (data->prev_brake && !data->brake);
        const bool accelerator_pressed = (!data->prev_accel && data->accel);

        if (brake_released || accelerator_pressed) {
            /* Battery check */
            if (data->batt_volt >= MIN_BATTERY_VOLTAGE && 
                data->batt_soc >= MIN_BATTERY_SOC) {
                send_encrypted_message(sock, "RESTART", CAN_ID_ECU_RESTART);
                log_toggle_event("Engine On");
                clock_gettime(CLOCK_MONOTONIC, restart_start);
                *is_restarting = true;
            } else {
                log_toggle_event("Fault: SWR3.5 (Low Battery)");
            }
        }
    }

    /* Update previous states */
    data->prev_brake = data->brake;
    data->prev_accel = data->accel;

    /* Restart monitoring */
    if (*is_restarting) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        const long elapsed_us = 
            ((now.tv_sec - restart_start->tv_sec) * MICROSECS_IN_ONESEC) + 
            ((now.tv_nsec - restart_start->tv_nsec) / NANO_TO_MICRO);

        if (elapsed_us > MICROSECS_IN_ONESEC) {
            *is_restarting = false;  // Successful restart
        } else if (data->batt_volt < MIN_BATTERY_VOLTAGE) {
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
                &restart_start_time
            );
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

#define CAN_INTERFACE ("vcan0")
#define CAN_DATA_LENGTH (8)
#define LOG_MESSAGE_SIZE (50)
#define SUCCESS_CODE (0)
#define ERROR_CODE (1)

int sock = -1;

bool check_is_valid_can_id(canid_t can_id)
{
    bool is_valid = false;

    switch (can_id)
    {
    case CAN_ID_COMMAND:
        is_valid = true;
        break;
    default:
        break;
    }

    return is_valid;
}

void parse_input_received(char *input)
{
    if (strcmp(input, "press_start_stop") == 0)
    {
        start_stop_manual = !start_stop_manual;

        if (start_stop_manual)
        {
            log_toggle_event("Stop/Start: System Activated");
        }
        else
        {
            log_toggle_event("Stop*/Start: System Deactivated");
        }
    }
    /* Check if CAN message is speed */
    if (sscanf(input, "speed: %lf", &rec_data.speed) == 1)
    {
        printf("received speed = %lf\n", rec_data.speed);
    }
    /* Check if CAN message is internal temperature */
    if (sscanf(input, "in_temp: %d", &rec_data.internal_temp) == 1)
    {
        printf("received in_temp = %d\n", rec_data.internal_temp);
    }
    /* Check if CAN message is external temperature */
    if (sscanf(input, "ex_temp: %d", &rec_data.external_temp) == 1)
    {
        printf("received ex_temp = %d\n", rec_data.external_temp);
    }
    /* Check if CAN message is door open */
    if (sscanf(input, "door: %d", &rec_data.door_open) == 1)
    {
        printf("received door = %d\n", rec_data.door_open);
    }
    /* Check if CAN message is tilt angle */
    if (sscanf(input, "tilt: %lf", &rec_data.tilt_angle) == 1)
    {
        printf("received tilt = %lf\n", rec_data.tilt_angle);
    }
    /* Check if CAN message is acceleration sensor */
    if (sscanf(input, "accel: %d", &rec_data.accel) == 1)
    {
        printf("received accel = %d\n", rec_data.accel);
    }
    /* Check if CAN message is braking sensor*/
    if (sscanf(input, "brake: %d", &rec_data.brake) == 1)
    {
        printf("received brake = %d\n", rec_data.brake);
    }
    /* Check if CAN message is temperature setpoint */
    if (sscanf(input, "temp_set: %d", &rec_data.temp_set) == 1)
    {
        printf("received temp_set = %d\n", rec_data.temp_set);
    }
    /* Check if CAN message is battery SoC */
    if (sscanf(input, "batt_soc: %lf", &rec_data.batt_soc) == 1)
    {
        printf("received batt_soc = %lf\n", rec_data.batt_soc);
    }
    /* Check if CAN message is battery voltage */
    if (sscanf(input, "batt_volt: %lf", &rec_data.batt_volt) == 1)
    {
        printf("received batt_volt = %lf\n", rec_data.batt_volt);
    }
    /* Check if CAN message is engine temperature */
    if (sscanf(input, "engi_temp: %lf", &rec_data.engi_temp) == 1)
    {
        printf("received engi_temp = %lf\n", rec_data.engi_temp);
    }
    /* Check if CAN message is gear */
    if (sscanf(input, "gear: %d", &rec_data.gear) == 1)
    {
        printf("received gear = %d\n", rec_data.gear);
    }
}

void process_received_frame(int sock)
{
    struct can_frame frame;
    unsigned char encrypted_data[AES_BLOCK_SIZE];
    char decrypted_message[AES_BLOCK_SIZE];
    int received_bytes = 0;
    char message_log[LOG_MESSAGE_SIZE];

    for (;;)
    {
        if (receive_can_frame(sock, &frame) == 0)
        {
            if (check_is_valid_can_id(frame.can_id))
            {
                (void)printf("Received CAN ID: %X Data: ", frame.can_id);
                for (int i = 0; i < frame.can_dlc; i++)
                {
                    (void)printf("%02X ", frame.data[i]);
                }
                (void)printf("\n");
                (void)fflush(stdout);

                if (frame.can_dlc == CAN_DATA_LENGTH)
                {
                    memcpy(encrypted_data + received_bytes, frame.data, CAN_DATA_LENGTH);
                    received_bytes += CAN_DATA_LENGTH;

                    if (received_bytes == AES_BLOCK_SIZE)
                    {
                        decrypt_data(encrypted_data, decrypted_message, received_bytes);
                        parse_input_received(decrypted_message);
                        received_bytes = 0;
                    }
                }
                else
                {
                    (void)printf("Warning: Unexpected frame size (%d bytes). Ignoring.\n", frame.can_dlc);
                    (void)fflush(stdout);
                }
            }
        }
    }
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