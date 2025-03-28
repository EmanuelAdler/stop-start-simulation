#include "bcm_func.h"

#define MICRO_CONSTANT_CONV  (1000000L)
#define NANO_CONSTANT_CONV   (1000)
#define CSV_LINE_BUFFER      (256)
#define CSV_MAX_FIELDS       (7)
#define CSV_MAX_TOKEN_SIZE   (50)
#define CSV_NUM_FIELD_0      (0)
#define CSV_NUM_FIELD_1      (1)
#define CSV_NUM_FIELD_2      (2)
#define CSV_NUM_FIELD_3      (3)
#define CSV_NUM_FIELD_4      (4)
#define CSV_NUM_FIELD_5      (5)
#define CSV_NUM_FIELD_6      (6)
#define THREAD_SLEEP_TIME    (100000U)
#define BATTERY_VOLT_MUL     (0.01125f)
#define BATTERY_VOLT_SUM     (11.675f)

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
int sock = -1;
char send_msg[AES_BLOCK_SIZE + 1] = {0};
bool test_mode = false;

// Sleep for a given number of microseconds
void sleep_microseconds(long int microseconds)
{
    struct timespec tsc;
    tsc.tv_sec = microseconds / MICRO_CONSTANT_CONV;
    tsc.tv_nsec = (microseconds % MICRO_CONSTANT_CONV) * NANO_CONSTANT_CONV;
    nanosleep(&tsc, NULL);
}

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
            // Remove surrounding quotes if any
            if (clean_token[0] == '"' && clean_token[strlen(clean_token) - 1] == '"')
            {
                memmove(clean_token, clean_token + 1, strlen(clean_token) - 1);
                clean_token[strlen(clean_token) - 1] = '\0';
            }
            // Replace comma with dot in numeric values
            for (char *p_aux = clean_token; *p_aux; p_aux++)
            {
                if (*p_aux == ',')
                {
                    *p_aux = '.';
                }
            }
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

// Check the simulation order and update state accordingly
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
                }
                else if (simu_state == STATE_PAUSED)
                {
                    simu_state = STATE_RUNNING;
                }
                break;
            case ORDER_PAUSE:
                if (simu_state == STATE_RUNNING)
                {
                    simu_state = STATE_PAUSED;
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
            if (*(controls.speed[simu_curr_step + 1]) - *(controls.speed[simu_curr_step]) > 0)
            {
                *(controls.accel[simu_curr_step]) = 1;
                *(controls.brake[simu_curr_step]) = 0;
                *(controls.gear[simu_curr_step]) = DRIVE;
            }
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
        /*
        printf("Time: %d, Speed: %.1f, Int Temp: %d, Ext Temp: %d, Door: %d, Tilt: %.1f, Accel: %d, Brake: %d, Set Temp: %d, Batt SOC: %.1f, Batt Volt: %.1f, Eng Temp: %.1f, Gear: %d\n",
                sim_data[simu_curr_step].time,
                sim_data[simu_curr_step].speed,
                sim_data[simu_curr_step].internal_temp,
                sim_data[simu_curr_step].external_temp,
                sim_data[simu_curr_step].door_open,
                sim_data[simu_curr_step].tilt_angle,
                sim_data[simu_curr_step].accel,
                sim_data[simu_curr_step].brake,
                sim_data[simu_curr_step].temp_set,
                sim_data[simu_curr_step].batt_soc,
                sim_data[simu_curr_step].batt_volt,
                sim_data[simu_curr_step].engi_temp,
                sim_data[simu_curr_step].gear);
        */
        if (simu_curr_step + 1 == data_size)
        {
            simu_order = ORDER_STOP;
        }
        simu_curr_step++;
    }
}

// Thread function to simulate speed updates
void* simu_speed(void *arg)
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

    ControlData control_data = { speed, accel, brake, gear };

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
    if (vehicle_data[simu_curr_step].speed != vehicle_data[simu_curr_step - 1].speed)
    {
        snprintf(send_msg, sizeof(send_msg), "speed: %.1lf", vehicle_data[simu_curr_step].speed);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].internal_temp != vehicle_data[simu_curr_step - 1].internal_temp)
    {
        snprintf(send_msg, sizeof(send_msg), "in_temp: %d", vehicle_data[simu_curr_step].internal_temp);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].external_temp != vehicle_data[simu_curr_step - 1].external_temp)
    {
        snprintf(send_msg, sizeof(send_msg), "ex_temp: %d", vehicle_data[simu_curr_step].external_temp);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].door_open != vehicle_data[simu_curr_step - 1].door_open)
    {
        snprintf(send_msg, sizeof(send_msg), "door: %d", vehicle_data[simu_curr_step].door_open);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].tilt_angle != vehicle_data[simu_curr_step - 1].tilt_angle)
    {
        snprintf(send_msg, sizeof(send_msg), "tilt: %.1lf", vehicle_data[simu_curr_step].tilt_angle);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].accel != vehicle_data[simu_curr_step - 1].accel)
    {
        snprintf(send_msg, sizeof(send_msg), "accel: %d", vehicle_data[simu_curr_step].accel);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].brake != vehicle_data[simu_curr_step - 1].brake)
    {
        snprintf(send_msg, sizeof(send_msg), "brake: %d", vehicle_data[simu_curr_step].brake);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].temp_set != vehicle_data[simu_curr_step - 1].temp_set)
    {
        snprintf(send_msg, sizeof(send_msg), "temp_set: %d", vehicle_data[simu_curr_step].temp_set);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].batt_soc != vehicle_data[simu_curr_step - 1].batt_soc)
    {
        snprintf(send_msg, sizeof(send_msg), "batt_soc: %.1lf", vehicle_data[simu_curr_step].batt_soc);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].batt_volt != vehicle_data[simu_curr_step - 1].batt_volt)
    {
        snprintf(send_msg, sizeof(send_msg), "batt_volt: %.1lf", vehicle_data[simu_curr_step].batt_volt);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].engi_temp != vehicle_data[simu_curr_step - 1].engi_temp)
    {
        snprintf(send_msg, sizeof(send_msg), "engi_temp: %.1lf", vehicle_data[simu_curr_step].engi_temp);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
    if (vehicle_data[simu_curr_step].gear != vehicle_data[simu_curr_step - 1].gear)
    {
        snprintf(send_msg, sizeof(send_msg), "gear: %d", vehicle_data[simu_curr_step].gear);
        send_encrypted_message(sock, send_msg, CAN_ID_SENSOR_READ);
    }
}

// Communication thread function
void* comms(void *arg)
{
    (void) arg;
    while (!test_mode)
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0)
        {
            return NULL;
        }
        if (simu_state == STATE_RUNNING)
        {
            send_data_update();
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
    } 
    else 
    {
        batt_soc -= BATTERY_SOC_DECREMENT;
        if (batt_soc < 0)
        {
            batt_soc = 0;
        }
    }
    batt_volt = (BATTERY_VOLT_MUL * batt_soc) + BATTERY_VOLT_SUM;
    vehicle_data[simu_curr_step].batt_soc = batt_soc;
    vehicle_data[simu_curr_step].batt_volt = batt_volt;
}

// Battery sensor thread function
void* sensor_battery(void *arg)
{
    (void) arg;
    while (!test_mode) 
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
