#include "bcm_func.h"

// CAN receiver
#define CAN_DATA_LENGTH (8)

#define MICRO_CONSTANT_CONV (1000000L)
#define NANO_CONSTANT_CONV (1000)
#define CSV_LINE_BUFFER (256)
#define CSV_MAX_FIELDS (7)
#define CSV_MAX_TOKEN_SIZE (50)
#define CSV_NUM_FIELD_0 (0)
#define CSV_NUM_FIELD_1 (1)
#define CSV_NUM_FIELD_2 (2)
#define CSV_NUM_FIELD_3 (3)
#define CSV_NUM_FIELD_4 (4)
#define CSV_NUM_FIELD_5 (5)
#define CSV_NUM_FIELD_6 (6)
#define THREAD_SLEEP_TIME (1000000U)
#define BATTERY_VOLT_MUL (0.01125f)
#define BATTERY_VOLT_SUM (11.675f)
#define BATTERY_SOC_MUL (5.0f)
#define VOLTAGE_OFFSET_VALUE (-0.8f)
#define SOC_THRESHOLD (30.0f)
#define VOLTAGE_DEC (0.5f)
#define SAFETY_TIMEOUT (2000)
#define SEC_TO_MS  1000ULL       // Seconds -> milliseconds
#define NSEC_TO_MS 1000000ULL    // Nanoseconds -> microseconds (in ms)
#define DOOR_OK_STATUS_1 (0)
#define DOOR_OK_STATUS_2 (1)
#define ENGINE_TEMP_OK_STATUS (120.0)
#define TILT_OK_STATUS (60.0)

// Global variables definitions
pthread_mutex_t mutex_bcm;
volatile int simu_curr_step = 0;
volatile int simu_state = STATE_STOPPED;
volatile int simu_order = ORDER_STOP;
bool curr_gear = PARKING;
float batt_volt = DEFAULT_BATTERY_VOLTAGE;
float batt_soc = DEFAULT_BATTERY_SOC;
int data_size = 0;
VehicleData vehicle_data[SPEED_ARRAY_MAX_SIZE] = {0};
int sock_send = -1;
int sock_recv = -1;
char send_msg[AES_BLOCK_SIZE + 1] = {0};
bool test_mode = false;
sem_t sem_comms;
bool fault_active = false;
int fault_start_time = 0;
const int safety_timeout_ms = SAFETY_TIMEOUT;
bool data_updated = false;

// Sleep for a given number of microseconds
void sleep_microseconds(long int microseconds)
{
    struct timespec tsc;
    tsc.tv_sec = microseconds / MICRO_CONSTANT_CONV;
    tsc.tv_nsec = (microseconds % MICRO_CONSTANT_CONV) * NANO_CONSTANT_CONV;
    nanosleep(&tsc, NULL);
}

int getCurrentTimeMs_real(void)
{
    struct timespec tss;
    clock_gettime(CLOCK_MONOTONIC, &tss);

    unsigned long long wideMs = ((unsigned long long)tss.tv_sec * SEC_TO_MS)
                     + (tss.tv_nsec / NSEC_TO_MS);

    return (int)wideMs;
}
#ifdef UNIT_TEST
    int mock_time_ms = 0;
    int getCurrentTimeMs(void) { return mock_time_ms; }
#else
    int getCurrentTimeMs(void) { return getCurrentTimeMs_real(); }
#endif

void read_csv_default(void)
{
    read_csv("../src/bcm/full_simu.csv");
}

// Read simulation data
void read_csv(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        perror("Error opening file");
        return;
    }

    char line[CSV_LINE_BUFFER];
    char temp_line[CSV_LINE_BUFFER];
    char *token;
    // Skip CSV header
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file))
    {
        strncpy(temp_line, line, sizeof(temp_line));
        temp_line[strcspn(temp_line, "\n")] = '\0';

        int field = 0;
        char *saveptr;
        token = strtok_r(temp_line, ",", &saveptr);

        while (token != NULL && field < CSV_MAX_FIELDS)
        {
            char clean_token[CSV_MAX_TOKEN_SIZE];
            strncpy(clean_token, token, sizeof(clean_token));
            
            switch (field)
            {
            case CSV_NUM_FIELD_0:
                vehicle_data[data_size].time = atoi(clean_token);
                break;
            case CSV_NUM_FIELD_1:
                vehicle_data[data_size].speed = atof(clean_token);
                break;
            case CSV_NUM_FIELD_2:
                vehicle_data[data_size].tilt_angle = atof(clean_token);
                break;
            case CSV_NUM_FIELD_3:
                vehicle_data[data_size].internal_temp = atoi(clean_token);
                break;
            case CSV_NUM_FIELD_4:
                vehicle_data[data_size].external_temp = atoi(clean_token);
                break;
            case CSV_NUM_FIELD_5:
                vehicle_data[data_size].door_open = atoi(clean_token);
                break;
            case CSV_NUM_FIELD_6:
                vehicle_data[data_size].engi_temp = atof(clean_token);
                break;
            default:
                break;
            }
            token = strtok_r(NULL, ",", &saveptr);
            field++;
        }
        data_size++;
    }
    data_size--;
    fclose(file);
}

// Check the simulation order and update the state accordingly
void check_order(int order)
{
    if (order != simu_state)
    {
        switch (order)
        {
        case ORDER_STOP:
            if (simu_state == STATE_RUNNING || simu_state == STATE_PAUSED)
            {
                simu_state = STATE_STOPPED;
                printf("Simulation Stopped!\n");
                fflush(stdout);
            }
            break;
        case ORDER_RUN:
            if (simu_state == STATE_STOPPED)
            {
                simu_curr_step = 0;
                memset(vehicle_data, 0, sizeof(vehicle_data));
                data_size = 0;
                read_csv_default();
                simu_state = STATE_RUNNING;
                printf("Simulation Running!\n");
                fflush(stdout);
            }
            else if (simu_state == STATE_PAUSED)
            {
                simu_state = STATE_RUNNING;
                printf("Simulation Running!\n");
                fflush(stdout);
            }
            break;
        case ORDER_PAUSE:
            if (simu_state == STATE_RUNNING)
            {
                simu_state = STATE_PAUSED;
                printf("Simulation Paused!\n");
                fflush(stdout);
            }
            break;
        default:
            break;
        }
    }
}

void simu_speed_step(VehicleData *sim_data, ControlData controls)
{
    if (simu_state == STATE_RUNNING)
    {
        if (simu_curr_step + 1 != data_size)
        {
            // Accelerating OR Constant speed, with speed > 0
            if (*(controls.speed[simu_curr_step + 1]) - *(controls.speed[simu_curr_step]) > 0.0
                ||
                ((*(controls.speed[simu_curr_step + 1]) == *(controls.speed[simu_curr_step])) &&
                *(controls.speed[simu_curr_step]) > 0.0))
            {
                *(controls.accel[simu_curr_step]) = 1;
                *(controls.brake[simu_curr_step]) = 0;
                *(controls.gear[simu_curr_step]) = DRIVE;
            }
            // Braking
            else if (*(controls.speed[simu_curr_step + 1]) - *(controls.speed[simu_curr_step]) < 0.0 && *(controls.speed[simu_curr_step]) > 0.0)
            {
                *(controls.brake[simu_curr_step]) = 1;
                *(controls.accel[simu_curr_step]) = 0;
                *(controls.gear[simu_curr_step]) = DRIVE;
            }
            // Stopped
            else
            {
                *(controls.brake[simu_curr_step]) = 1;
                *(controls.accel[simu_curr_step]) = 0;
                if (sim_data[simu_curr_step].speed == 0)
                {
                    *(controls.gear[simu_curr_step]) = PARKING;
                }
            }
        }

        data_updated = true;

        if (simu_curr_step + 1 == data_size)
        {
            simu_order = ORDER_STOP;
        }
        sem_post(&sem_comms);
    }
}

// Thread function to simulate speed updates
void *simu_speed(void *arg)
{
    VehicleData *sim_data = (VehicleData *)arg;
    check_order(simu_order);

    double *speed[data_size];
    int *accel[data_size];
    int *brake[data_size];
    int *gear[data_size];
    int *temp_set[data_size];

    for (int i = 0; i < data_size; i++)
    {
        speed[i] = &sim_data[i].speed;
        accel[i] = &sim_data[i].accel;
        brake[i] = &sim_data[i].brake;
        gear[i] = &sim_data[i].gear;
        temp_set[i] = &sim_data[i].temp_set;
        *temp_set[i] = DEFAULT_SET_TEMP;
    }

    ControlData control_data = {speed, accel, brake, gear};

    while (!test_mode)
    {
        pthread_mutex_lock(&mutex_bcm);
        check_order(simu_order);
        simu_speed_step(sim_data, control_data);
        pthread_mutex_unlock(&mutex_bcm);
        sleep_microseconds(THREAD_SLEEP_TIME);
    }
    return NULL;
}

// Function to check for updates in simulation data and send CAN messages
void send_data_update(void)
{
    snprintf(send_msg, sizeof(send_msg), "speed: %.1lf", vehicle_data[simu_curr_step].speed);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "in_temp: %d", vehicle_data[simu_curr_step].internal_temp);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "ex_temp: %d", vehicle_data[simu_curr_step].external_temp);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "door: %d", vehicle_data[simu_curr_step].door_open);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "tilt: %.1lf", vehicle_data[simu_curr_step].tilt_angle);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "accel: %d", vehicle_data[simu_curr_step].accel);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "brake: %d", vehicle_data[simu_curr_step].brake);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "temp_set: %d", vehicle_data[simu_curr_step].temp_set);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "batt_soc: %.1lf", vehicle_data[simu_curr_step].batt_soc);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "batt_volt: %.1lf", vehicle_data[simu_curr_step].batt_volt);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "engi_temp: %.1lf", vehicle_data[simu_curr_step].engi_temp);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);

    snprintf(send_msg, sizeof(send_msg), "gear: %d", vehicle_data[simu_curr_step].gear);
    send_encrypted_message(sock_send, send_msg, CAN_ID_SENSOR_READ);
}

// Check if can_id is valid
bool check_can_id(canid_t can_id)
{
    bool is_valid = false;

    switch (can_id)
    {
    case CAN_ID_COMMAND:
    //case CAN_ID_ERROR_DASH:
        is_valid = true;
        break;
    default:
        break;
    }

    return is_valid;
}

/* Function to check if system was disabled by another ECU,
so that simulation will halt. */
void check_system_disable(int sock)
{
    struct can_frame frame;
    unsigned char encrypted_data[AES_BLOCK_SIZE];
    char decrypted_message[AES_BLOCK_SIZE];
    int received_bytes = 0;
    char error_log[MAX_MSG_SIZE];

    if (receive_can_frame(sock, &frame) == 0)
    {
        if (check_can_id(frame.can_id))
        {
            if (frame.can_dlc == CAN_DATA_LENGTH)
            {
                memcpy(encrypted_data + received_bytes, frame.data, CAN_DATA_LENGTH);
                received_bytes += CAN_DATA_LENGTH;

                if (received_bytes == AES_BLOCK_SIZE)
                {
                    decrypt_data(encrypted_data, decrypted_message, received_bytes);
                    //parse_input_received(decrypted_message);

                    // if system is disabled, order simulation to stop
                    if (strcmp(decrypted_message, "system_disabled_error") == 0)
                    {
                        check_order(ORDER_STOP);
                    }
                    received_bytes = 0;
                }
            }
            else
            {
                snprintf(error_log, sizeof(error_log), "Warn: Frame ignored (size %u bytes > 8).", frame.can_dlc);
            }
        }
    }
}

// Function to evaluate system health
void check_health_signals(void)
{
    int doorVal         = vehicle_data[simu_curr_step].door_open;
    double engTemp      = vehicle_data[simu_curr_step].engi_temp;
    double tilt         = vehicle_data[simu_curr_step].tilt_angle;

    bool invalidDoor = (doorVal != DOOR_OK_STATUS_1 && doorVal != DOOR_OK_STATUS_2);

    bool engineOvertemp = (engTemp > ENGINE_TEMP_OK_STATUS);

    bool excessiveTilt = (tilt > TILT_OK_STATUS);

    bool anyAdverseCondition = (invalidDoor || engineOvertemp || excessiveTilt);

    if (anyAdverseCondition)
    {
        if (!fault_active)
        {
            fault_active = true;
            fault_start_time = getCurrentTimeMs(); 
        }
        else
        {
            int elapsed = getCurrentTimeMs() - fault_start_time;
            if (elapsed >= safety_timeout_ms)
            {
                send_encrypted_message(sock_send, "system_disabled_error", CAN_ID_COMMAND);
                log_toggle_event("Fault: SWR6.4 (System Disabling Error)");
                if (invalidDoor)
                {
                    log_toggle_event("Fault: SWR6.4 (Invalid door status)");
                }
                if (engineOvertemp)
                {
                    log_toggle_event("Fault: SWR6.4 (Engine overtemperature)");
                }
                if (excessiveTilt)
                {
                    log_toggle_event("Fault: SWR6.4 (Excessive tilt value)");
                }

                simu_order = ORDER_STOP;
            }
        }
    }
    else
    {
        fault_active = false;
    }
}

// Communication thread function
void *comms(void *arg)
{
    (void)arg;
    #ifdef UNIT_TEST
        int max_iterations = 2;
        int iter;
        for (iter = 0; iter < max_iterations; ++iter)
    #else
        while (!test_mode)
    #endif
    {
        sem_wait(&sem_comms);

        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0)
        {
            return NULL;
        }
        if (simu_state == STATE_RUNNING && data_updated)
        {
            send_data_update();
            data_updated = false; // ready to update data again after sending
            simu_curr_step++;
            check_health_signals();
            // Check if it's necessary to stop simulation
            check_system_disable(sock_recv);
        }
        pthread_mutex_unlock(&mutex_bcm);
        sleep_microseconds(THREAD_SLEEP_TIME);
    }
    return NULL;
}

// Update battery state of charge based on vehicle speed
void update_battery_soc(double vehicle_speed)
{
    if (vehicle_speed > 0.0)
    {
        batt_soc += BATTERY_SOC_INCREMENT;
        if (batt_soc > MAX_BATTERY_SOC)
        {
            batt_soc = MAX_BATTERY_SOC;
        }
        batt_volt = (BATTERY_VOLT_MUL * batt_soc) + BATTERY_VOLT_SUM;
    }
    else
    {
        batt_soc -= (BATTERY_SOC_DECREMENT * BATTERY_SOC_MUL);
        if (batt_soc < 0)
        {
            batt_soc = 0;
        }

        batt_volt = (BATTERY_VOLT_MUL * batt_soc) + BATTERY_VOLT_SUM + VOLTAGE_OFFSET_VALUE;

        if (batt_soc < SOC_THRESHOLD)
        {
            batt_volt -= VOLTAGE_DEC;
        }
    }

    vehicle_data[simu_curr_step].batt_soc = batt_soc;
    vehicle_data[simu_curr_step].batt_volt = batt_volt;
}

// Battery sensor thread function
void *sensor_battery(void *arg)
{
    (void)arg;
#ifdef UNIT_TEST
    // Bounded loop
    int max_iterations = 2;
    int i;
    for (i = 0; i < max_iterations; i++)
#else
    while (!test_mode)
#endif
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0)
        {
            return NULL;
        }
        if (simu_state == STATE_RUNNING)
        {
            update_battery_soc(vehicle_data[simu_curr_step].speed);
        }
        pthread_mutex_unlock(&mutex_bcm);
        sleep_microseconds(THREAD_SLEEP_TIME);
    }
    return NULL;
}
