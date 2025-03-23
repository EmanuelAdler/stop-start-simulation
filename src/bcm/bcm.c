#include "bcm.h"

#define SLEEP_TIME_US (100000U)
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

/***** Speed sensor data *****/

#define SPEED_ARRAY_MAX_SIZE 5000

/* Current speed */
// static double curr_speed = 0.0F;

/* Current speed array */
static double speed_array[SPEED_ARRAY_MAX_SIZE] = {0}; // Initialize array with zeros

/* Number of data points in csv file */
static int data_size = 0;

/***** Accelerator pedal sensor data *****/

static bool is_accelerating = false;

/***** Brake sensor data *****/

static bool is_braking = false;

/***** Inclinometer sensor data *****/

/* Current vehicle angle - horizontal reference */
static float curr_vehicle_angle = 0.0F;

/***** Battery sensor data *****/

#define DEFAULT_BATTERY_VOLTAGE 12.0F
#define DEFAULT_BATTERY_SOC 80.0F

static float bat_voltage = DEFAULT_BATTERY_VOLTAGE;
static float bat_soc = DEFAULT_BATTERY_SOC;

/***** AC sensor data *****/

#define DEFAULT_OUTSIDE_TEMP 28.0F
#define DEFAULT_INSIDE_TEMP 25.0F
#define DEFAULT_SET_TEMP 23.0F

static float out_temp = DEFAULT_OUTSIDE_TEMP;
static float in_temp = DEFAULT_INSIDE_TEMP;
static float set_temp = DEFAULT_SET_TEMP;

/***** Door sensor data *****/

/* Any door opened */
static bool opened_door = false;

/***** Function to get data from csv file *****/

void read_csv()
{
#define MAX_LINE_SIZE 100
#define MAX_SPEED_STRING_SIZE 20

    FILE *file = fopen("ftp75.csv", "r");
    if (!file)
    {
        perror("Error opening file");
    }

    int speed_filled[SPEED_ARRAY_MAX_SIZE] = {0}; // Auxiliary array to track filled indices
    int indice;
    char read_string_value[MAX_SPEED_STRING_SIZE];
    double final_value;
    char file_line[MAX_LINE_SIZE];

    data_size = 0;

    // Skip the header line
    fgets(file_line, sizeof(file_line), file);

    // Read the file line by line
    while (fgets(file_line, sizeof(file_line), file))
    {
        // Parse index and value from the line
        if (sscanf(file_line, "%d,%[^\n]", &indice, read_string_value) == 2)
        {
            // Check if the value is enclosed in quotes
            if (read_string_value[0] == '"' && read_string_value[strlen(read_string_value) - 1] == '"')
            {
                // Remove quotes
                memmove(read_string_value, read_string_value + 1, strlen(read_string_value)); // Remove first quote
                read_string_value[strlen(read_string_value) - 1] = '\0';                      // Remove last quote

                // Replace comma with dot for proper decimal conversion
                for (char *rep = read_string_value; *rep; rep++)
                {
                    if (*rep == ',')
                    {
                        *rep = '.';
                    }
                }
            }

            final_value = atof(read_string_value); // Convert to double

            // Store the value in the array if the index is valid
            if (indice >= 0 && indice < SPEED_ARRAY_MAX_SIZE)
            {
                speed_array[indice] = final_value;
                speed_filled[indice] = 1; // Mark the index as filled
                data_size++;
            }
        }
    }

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
                memset(speed_array, 0, sizeof(speed_array)); // delete current simulation speed data set
                simu_curr_step = 0;                          // reset the current simulation step
                simu_state = STATE_STOPPED;
            }

            break;

        case ORDER_RUN:

            if (simu_state == STATE_STOPPED)
            {
                read_csv(); // acquire new set of speed data to simulate from start
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

static void *simu_speed(void *arg[])
{
    int *simu_order = (int *)arg[0];
    double *speed = (double *)arg[1];

    while (true)
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0)
        {
            return NULL;
        }

        // check if any order is received

        check_order(*simu_order);

        // continue simulation if it's running

        if (simu_state == STATE_RUNNING)
        {
            *speed = speed_array[simu_curr_step];
            printf("([%d] of [%d]) current speed = %lf\n", simu_curr_step + 1, data_size, *speed);
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