#include "simu_sensors.h"

#define SLEEP_TIME_US           (100000U)
#define MICROSECS_IN_ONESEC     (1000000L)
#define NANO_TO_MICRO           (1000)

void sleep_microseconds(long int msec) {
    struct timespec tspec;
    tspec.tv_sec = msec / MICROSECS_IN_ONESEC;
    tspec.tv_nsec = (msec % MICROSECS_IN_ONESEC) * NANO_TO_MICRO;
    nanosleep(&tspec, NULL);
}

/***** Speed sensor data *****/

/* Kilometers per hour to meters per second ratio */
#define M_S_KM_H_RATIO        (3.6f)

/* Constant to convert vehicle speed from rad/s to m/s */
#define PI_OVER_30            (3.1415f / 30.0f)

/* Wheel radius, in meters */
#define WHEEL_RADIUS          (0.326f)

/* 1/(differential ratio) */
#define ONE_OVER_ND           (0.2817f)

/* 1/(current gear ratio) */
static float one_over_curr_ng = 0.0F;

/* Current speed */
static float vel_atual = 0.0F;

/***** Accelerator pedal sensor data *****/

static bool is_accelerating = true;

/***** Brake sensor data *****/

static bool is_braking = false;

/***** Gear sensor data *****/

#define MAX_RPM 5000

#define NG_1    0.2611F
#define NG_2    0.4237F
#define NG_3    0.5917F
#define NG_4    0.7634F
#define NG_5    1.00F

typedef enum {
    GEAR_P = 0,
    GEAR_1 = 1,
    GEAR_2 = 2,
    GEAR_3 = 3,
    GEAR_4 = 4,
    GEAR_5 = 5,
    GEAR_INVALID
} Gear;

/* Current gear */
static Gear gear = GEAR_P;

/* Current engine RPM */
static int curr_RPM = 0;

/* Current gear variation step */
static int curr_step = 1;

/* Number of simulated steps which cause gear to change */
static const int num_sim_steps = 5;

/* RPM loss when changing gear */
static const int RPM_gear_loss = 1200;

/* RPM increment step when accelerating */
static const int RPM_increment_step = 300;

/* RPM decrement step when braking */
static const int RPM_decrement_step = 450;

/***** Inclinometer sensor data *****/

/* Current vehicle angle - horizontal reference */
static float curr_vehicle_angle = 0.0F;

/***** Engine coolant sensor data *****/

#define ACCEL_FACTOR          (0.1f)
#define AIR_COOL_FACTOR       (0.05f)
#define MAX_TEMP_MOTOR        (105.0f)
#define BASE_TEMP             (70.0f)

#define TEMP_MOD    (1 / 10.0f)

/* Current engine coolant temperature */
static float curr_cool_temp = 0.0F;

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

/***** Calculate engine temperature *****/

static float calculate_engine_temp(struct engine_temp etemp) 
{
    float temp_rise = ((float)etemp.engine_rpm * TEMP_MOD * ACCEL_FACTOR);
    float cooling_effect = etemp.vehicle_speed * AIR_COOL_FACTOR;
    float temp = BASE_TEMP + temp_rise - cooling_effect;
    return fminf(MAX_TEMP_MOTOR, temp);
}

/***** Calculate speed *****/

static void *simu_sensor_speed(void *arg) 
{
    float *speed = (float *)arg;

    while (true) 
    {
        int lock_result = pthread_mutex_lock(&mutex_sensors);
        if (lock_result != 0) 
        {
            return NULL;
        }

        switch (gear) 
        {
            case GEAR_1:
                one_over_curr_ng = NG_1;
                break;
            case GEAR_2:
                one_over_curr_ng = NG_2;
                break;
            case GEAR_3:
                one_over_curr_ng = NG_3;
                break;
            case GEAR_4:
                one_over_curr_ng = NG_4;
                break;
            case GEAR_5:
                one_over_curr_ng = NG_5;
                break;
            default:
                /* Handle unexpected gear value */
                break;
        }

        if(gear == GEAR_P){
            *speed = 0.00F;
        }
        else{
            *speed = ((float)curr_RPM * WHEEL_RADIUS * one_over_curr_ng * 
                 ONE_OVER_ND * PI_OVER_30 * M_S_KM_H_RATIO);
        }
        

        int unlock_result = pthread_mutex_unlock(&mutex_sensors);
        if (unlock_result != 0) 
        {
            return NULL;
        }

        sleep_microseconds(SLEEP_TIME_US);
    }
    return NULL;
}

/***** Calculate RPM *****/

static void *simu_sensor_gear_pos(void *arg) 
{
    int *rpm = (int *)arg;

    while (true) 
    {
        int lock_result = pthread_mutex_lock(&mutex_sensors);
        if (lock_result != 0) 
        {
            return NULL;
        }

        if (curr_step == num_sim_steps) 
        {
            if ((is_accelerating && !is_braking) == true) 
            {
                if (gear < GEAR_5) 
                {
                    gear++;
                }
            } 
            else 
            {
                if (gear > GEAR_P) 
                {
                    gear--;
                }
            }
            curr_step = 1;
        } 
        else 
        {
            curr_step++;
        }

        int delta = is_braking ? -RPM_decrement_step : RPM_increment_step;
        curr_RPM += delta;

        if (curr_RPM > MAX_RPM) 
        {
            curr_RPM = MAX_RPM;
        } 
        else if (curr_RPM < 0) 
        {
            curr_RPM = 0;
        }

        *rpm = curr_RPM;

        int unlock_result = pthread_mutex_unlock(&mutex_sensors);
        if (unlock_result != 0) 
        {
            return NULL;
        }

        sleep_microseconds(SLEEP_TIME_US);
    }
    return NULL;
}