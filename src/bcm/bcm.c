#include "bcm.h"

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

/***** Simulation settings *****/

#define ORDER_STOP 0
#define ORDER_RUN 1
#define ORDER_PAUSE 2

#define STATE_STOPPED 0
#define STATE_RUNNING 1
#define STATE_PAUSED 2

static int simu_curr_step = 0;

static int simu_state = STATE_STOPPED;

static int simu_order = ORDER_STOP;

/***** Gear data *****/

#define PARKING 0
#define DRIVE 1

/* Current gear | 0 - Parking | 1 - Drive | */
static bool curr_gear = PARKING;

/***** Speed sensor data *****/

#define SPEED_ARRAY_MAX_SIZE 5000

/* Number of data points in csv file */
static int data_size = 0;

/***** Battery sensor data *****/

#define DEFAULT_BATTERY_VOLTAGE 12.0F
#define DEFAULT_BATTERY_SOC 80.0F

static float batt_volt = DEFAULT_BATTERY_VOLTAGE;
static float batt_soc = DEFAULT_BATTERY_SOC;

/***** AC sensor data *****/

#define DEFAULT_SET_TEMP 23U

/***** Function to get data from csv file *****/

#define MAX_LINE_SIZE 256

#define CASE_TIME 0
#define CASE_SPEED 1
#define CASE_TILT 2
#define CASE_INTEMP 3
#define CASE_EXTEMP 4
#define CASE_DOOR 5
#define CASE_ENG_TEMP 6

#define CLEAN_STRING_SIZE 50

#define MAX_FIELD 7

/* Vehicle data structure*/

VehicleData vehicle_data[SPEED_ARRAY_MAX_SIZE] = {0};

void read_csv()
{
    FILE *file = fopen("full_simu.csv", "r");
    if (!file)
    {
        perror("Error opening file");
        return;
    }

    char line[MAX_LINE_SIZE];
    char temp_line[MAX_LINE_SIZE];
    char *token;

    // Bypass csv header
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file))
    {
        // Copy content for extraction
        strncpy(temp_line, line, sizeof(temp_line));
        temp_line[strcspn(temp_line, "\n")] = 0; // Remove newline

        // Extract each field
        int field = 0;
        char *saveptr;
        token = strtok_r(temp_line, ",", &saveptr);

        while (token != NULL && field < MAX_FIELD)
        {
            // Remove "" if exists
            char clean_token[CLEAN_STRING_SIZE];
            strncpy(clean_token, token, sizeof(clean_token));
            if (clean_token[0] == '"' && clean_token[strlen(clean_token) - 1] == '"')
            {
                memmove(clean_token, clean_token + 1, strlen(clean_token) - 1);
                clean_token[strlen(clean_token) - 1] = '\0';
            }

            // Substituir vírgula decimal por ponto
            for (char *rep = clean_token; *rep; rep++)
            {
                if (*rep == ',')
                {
                    *rep = '.';
                }
            }

            // Atribuir aos campos da estrutura
            switch (field)
            {
            case CASE_TIME:
                vehicle_data[data_size].time = atoi(clean_token);
                break;
            case CASE_SPEED:
                vehicle_data[data_size].speed = atof(clean_token);
                break;
            case CASE_TILT:
                vehicle_data[data_size].tilt_angle = atof(clean_token);
                break;
            case CASE_INTEMP:
                vehicle_data[data_size].internal_temp = atoi(clean_token);
                break;
            case CASE_EXTEMP:
                vehicle_data[data_size].external_temp = atoi(clean_token);
                break;
            case CASE_DOOR:
                vehicle_data[data_size].door_open = atoi(clean_token);
                break;
            case CASE_ENG_TEMP:
                vehicle_data[data_size].engi_temp = atof(clean_token);
                break;
            default:
                break;
            }

            // printf("field = %d\n", field);

            token = strtok_r(NULL, ",", &saveptr);
            field++;
        }
        data_size++;
    }
    data_size--;
    fclose(file);
}

/***** Function to check if order is received *****/
void check_order(int simu_order)
{
    if (simu_order != simu_state)
    {
        switch (simu_order)
        {
        case ORDER_STOP:

            if (simu_state == (STATE_RUNNING || STATE_PAUSED))
            {
                simu_state = STATE_STOPPED;
            }

            break;

        case ORDER_RUN:

            if (simu_state == STATE_STOPPED)
            {
                /* reset the current simulation step */
                simu_curr_step = 0;
                /* delete current simulation speed data set */
                memset(vehicle_data, 0, sizeof(vehicle_data));
                /* reset number of lines in csv*/
                data_size = 0;
                /* acquire new set of speed data to simulate from start */
                read_csv();

                simu_state = STATE_RUNNING;
            }
            if (STATE_PAUSED)
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

/***** Simulate speed *****/

static void *simu_speed(void *arg)
{
    VehicleData *ptr_simu_data = (VehicleData *)arg;

    check_order(simu_order);

    double *speed[data_size];
    int *accel[data_size];
    int *brake[data_size];
    int *gear[data_size];
    int *temp_set[data_size];

    for (int i = 0; i < data_size; i++)
    {
        speed[i] = &ptr_simu_data[i].speed;
        accel[i] = &ptr_simu_data[i].accel;
        brake[i] = &ptr_simu_data[i].brake;
        gear[i] = &ptr_simu_data[i].gear;
        temp_set[i] = &ptr_simu_data[i].temp_set;
        *temp_set[i] = DEFAULT_SET_TEMP;
    }

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0)
        {
            return NULL;
        }

        // check if any order is received

        check_order(simu_order);

        // continue simulation if it's running

        if (simu_state == STATE_RUNNING)
        {
            /* Check if car is accelerating or braking */

            if (simu_curr_step + 1 != data_size)
            {
                if (*speed[simu_curr_step + 1] - *speed[simu_curr_step] > 0)
                {
                    *accel[simu_curr_step] = true;
                    *brake[simu_curr_step] = false;

                    /* If car is moving, then gear = DRIVE */
                    *gear[simu_curr_step] = DRIVE;
                }
                else
                {
                    *brake[simu_curr_step] = true;
                    *accel[simu_curr_step] = false;

                    /* If car is not moving, gear = PARKING */
                    if (vehicle_data[simu_curr_step].speed == 0)
                    {
                        *gear[simu_curr_step] = PARKING;
                    }
                }
            }

            printf("Time: %d, Speed: %.1f, Int. Temp: %d, Ext. Temp: %d, Door: %d, Tilt: %.1f, Accel: %d, Brake: %d, Setp_temp = %d, Batt_soc = %.1f, Batt_volt = %.1f, Engine temp = %.1f, gear = %d\n",
                   vehicle_data[simu_curr_step].time,
                   vehicle_data[simu_curr_step].speed,
                   vehicle_data[simu_curr_step].internal_temp,
                   vehicle_data[simu_curr_step].external_temp,
                   vehicle_data[simu_curr_step].door_open,
                   vehicle_data[simu_curr_step].tilt_angle,
                   vehicle_data[simu_curr_step].accel,
                   vehicle_data[simu_curr_step].brake,
                   vehicle_data[simu_curr_step].temp_set,
                   vehicle_data[simu_curr_step].batt_soc,
                   vehicle_data[simu_curr_step].batt_volt,
                   vehicle_data[simu_curr_step].engi_temp,
                   vehicle_data[simu_curr_step].gear);

            if (simu_curr_step + 1 == data_size)
            {
                simu_order = ORDER_STOP;
            }
            simu_curr_step++;
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

/* CAN communication */

#define CAN_INTERFACE ("vcan0")
#define ERROR_CODE (1)

int sock = -1;

char send_msg[20];

static void send_data_update()
{

    /* Check speed update */
    if (vehicle_data[simu_curr_step].speed != vehicle_data[simu_curr_step - 1].speed)
    {
        snprintf(send_msg, sizeof(send_msg), "speed: %lf", vehicle_data[simu_curr_step].speed);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check internal temperature update */
    if (vehicle_data[simu_curr_step].internal_temp != vehicle_data[simu_curr_step - 1].internal_temp)
    {
        snprintf(send_msg, sizeof(send_msg), "in_temp: %d", vehicle_data[simu_curr_step].internal_temp);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check external temperature update */
    if (vehicle_data[simu_curr_step].external_temp != vehicle_data[simu_curr_step - 1].external_temp)
    {
        snprintf(send_msg, sizeof(send_msg), "ex_temp: %d", vehicle_data[simu_curr_step].external_temp);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check door opened update */
    if (vehicle_data[simu_curr_step].door_open != vehicle_data[simu_curr_step - 1].door_open)
    {
        snprintf(send_msg, sizeof(send_msg), "door: %d", vehicle_data[simu_curr_step].door_open);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check tilt angle update */
    if (vehicle_data[simu_curr_step].tilt_angle != vehicle_data[simu_curr_step - 1].tilt_angle)
    {
        snprintf(send_msg, sizeof(send_msg), "tilt: %lf", vehicle_data[simu_curr_step].tilt_angle);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check acceleration update */
    if (vehicle_data[simu_curr_step].accel != vehicle_data[simu_curr_step - 1].accel)
    {
        snprintf(send_msg, sizeof(send_msg), "accel: %d", vehicle_data[simu_curr_step].accel);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check brake update */
    if (vehicle_data[simu_curr_step].brake != vehicle_data[simu_curr_step - 1].brake)
    {
        snprintf(send_msg, sizeof(send_msg), "brake: %d", vehicle_data[simu_curr_step].brake);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    // check if necessary
    /* Check temperature setpoint update */
    if (vehicle_data[simu_curr_step].temp_set != vehicle_data[simu_curr_step - 1].temp_set)
    {
        snprintf(send_msg, sizeof(send_msg), "temp_set: %d", vehicle_data[simu_curr_step].temp_set);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check battery SoC update */
    if (vehicle_data[simu_curr_step].batt_soc != vehicle_data[simu_curr_step - 1].batt_soc)
    {
        snprintf(send_msg, sizeof(send_msg), "batt_soc: %lf", vehicle_data[simu_curr_step].batt_soc);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check battery voltage update */
    if (vehicle_data[simu_curr_step].batt_volt != vehicle_data[simu_curr_step - 1].batt_volt)
    {
        snprintf(send_msg, sizeof(send_msg), "batt_volt: %lf", vehicle_data[simu_curr_step].batt_volt);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check engine temperature update */
    if (vehicle_data[simu_curr_step].engi_temp != vehicle_data[simu_curr_step - 1].engi_temp)
    {
        snprintf(send_msg, sizeof(send_msg), "engi_temp: %lf", vehicle_data[simu_curr_step].engi_temp);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
    /* Check gear update */
    if (vehicle_data[simu_curr_step].gear != vehicle_data[simu_curr_step - 1].gear)
    {
        snprintf(send_msg, sizeof(send_msg), "gear: %d", vehicle_data[simu_curr_step].gear);
        send_encrypted_message(sock, send_msg, CAN_ID_COMMAND);
    }
}

static void *comms(void *arg)
{

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0)
        {
            return NULL;
        }

        /* Check for data update and send if updated, if simulation is running */
        if (simu_state == STATE_RUNNING)
        {
            send_data_update();
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

int main()
{

    sock = create_can_socket(CAN_INTERFACE);
    if (sock < 0)
    {
        return EXIT_FAILURE;
    }

    simu_order = ORDER_RUN;

    pthread_mutex_init(&mutex_bcm, NULL);

    pthread_t thread_speed;
    pthread_t thread_comms;

    pthread_create(&thread_speed, NULL, simu_speed, &vehicle_data);
    pthread_create(&thread_comms, NULL, comms, NULL);

    pthread_join(thread_speed, NULL);
    pthread_join(thread_comms, NULL);

    pthread_mutex_destroy(&mutex_bcm);

    close_can_socket(sock);

    return 0;
}