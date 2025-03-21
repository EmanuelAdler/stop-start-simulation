#include "bcm.h"

#define SLEEP_TIME_US           (100000U)
#define MICROSECS_IN_ONESEC     (1000000L)
#define NANO_TO_MICRO           (1000)

void sleep_microseconds(long int msec) {
    struct timespec tspec;
    tspec.tv_sec = msec / MICROSECS_IN_ONESEC;
    tspec.tv_nsec = (msec % MICROSECS_IN_ONESEC) * NANO_TO_MICRO;
    nanosleep(&tspec, NULL);
}

/***** Simulation settings *****/

#define ORDER_STOP      0
#define ORDER_RUN       1
#define ORDER_PAUSE     2 

#define STATE_STOPPED   0
#define STATE_RUNNING   1
#define STATE_PAUSED    2 

static int simu_curr_step = 0;

static int simu_state = STATE_STOPPED;

/***** Speed sensor data *****/



/* Current speed */
static float curr_speed = 0.0F;

/***** Accelerator pedal sensor data *****/

static bool is_accelerating = false;

/***** Brake sensor data *****/

static bool is_braking = false;

/***** Inclinometer sensor data *****/

/* Current vehicle angle - horizontal reference */
static float curr_vehicle_angle = 0.0F;


/***** Battery sensor data *****/

#define DEFAULT_BATTERY_VOLTAGE     12.0F
#define DEFAULT_BATTERY_SOC         80.0F

static float bat_voltage = DEFAULT_BATTERY_VOLTAGE;
static float bat_soc = DEFAULT_BATTERY_SOC;

/***** AC sensor data *****/

#define DEFAULT_OUTSIDE_TEMP       28.0F
#define DEFAULT_INSIDE_TEMP        25.0F
#define DEFAULT_SET_TEMP           23.0F

static float out_temp = DEFAULT_OUTSIDE_TEMP;
static float in_temp = DEFAULT_INSIDE_TEMP;
static float set_temp = DEFAULT_SET_TEMP;

/***** Door sensor data *****/

/* Any door opened */
static bool opened_door = false;

/***** Function to get data from csv file *****/

#define MAX_LINES   5000

int read_csv(){
    FILE *file = fopen("ftp75.csv", "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    double speed_array[MAX_LINES] = {0}; // Initialize array with zeros
    int speed_filled[MAX_LINES] = {0}; // Auxiliary array to track filled indices
    int indice;
    char read_string_value[20];
    double final_value;
    char file_line[100];

    // Skip the header line
    fgets(file_line, sizeof(file_line), file);

    // Read the file line by line
    while (fgets(file_line, sizeof(file_line), file)) {
        // Parse index and value from the line
        if (sscanf(file_line, "%d,%[^\n]", &indice, read_string_value) == 2) {
            // Check if the value is enclosed in quotes
            if (read_string_value[0] == '"' && read_string_value[strlen(read_string_value) - 1] == '"') {
                // Remove quotes
                memmove(read_string_value, read_string_value + 1, strlen(read_string_value)); // Remove first quote
                read_string_value[strlen(read_string_value) - 1] = '\0'; // Remove last quote

                // Replace comma with dot for proper decimal conversion
                for (char *p = read_string_value; *p; p++) {
                    if (*p == ',') {
                        *p = '.';
                    }
                }
            }

            final_value = atof(read_string_value); // Convert to double

            // Store the value in the array if the index is valid
            if (indice >= 0 && indice < MAX_LINES) {
                speed_array[indice] = final_value;
                speed_filled[indice] = 1; // Mark the index as filled
            }
        }
    }

    fclose(file);

 /*    // Exibe apenas os final_valuees armazenados no speed_array que foram speed_filleds
    printf("speed_array resultante:\n");
    for (int i = 0; i < MAX_SIZE; i++) {
        if (speed_filled[i]) { // Verifica se o índice foi speed_filled
            printf("speed_array[%d] = %.2lf\n", i, speed_array[i]);
        }
    } */

    return speed_array;
}

/***** Simulate speed *****/

static void *simu_speed(void *arg) 
{
    int *rpm = (int *)arg;

    while (true) 
    {
        int lock_result = pthread_mutex_lock(&mutex_bcm);
        if (lock_result != 0) 
        {
            return NULL;
        }

        // check if any order is received

        if(simu_order != simu_state){
            switch (simu_order)
            {
            case ORDER_STOP:

                if(simu_state == (STATE_RUNNING || STATE_PAUSED)){
                    simu_curr_step = 0;
                    simu_state = STATE_STOPPED;
                }
                
                break;

            case ORDER_RUN:

                if(simu_state == (STATE_STOPPED || STATE_PAUSED)){
                    simu_state = STATE_RUNNING;
                }

                break;

            case ORDER_PAUSE:

                if(simu_state == STATE_RUNNING){
                    simu_state = STATE_PAUSED;
                }

                break;
            
            default:
                break;
            }
        }

        if (simu_state == STATE_RUNNING)
        {
            
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