#include "simu_sensors.h"

/***** Speed sensor data *****/

// constant to convert vehicle speed from rad/s to m/s
#define PI_OVER_30 3.1415/30;

// wheel radius, in meters
#define WHEEL_RADIUS 0.326;

// 1/(differential ratio)
#define ONE_OVER_PI 0.2817;

// 1/(current gear ratio)
float um_sobre_n_g_atual = 0.00;

// current speed
float vel_atual = 0;

/***** Brake sensor data *****/

int is_braking = 0;

/***** Gear sensor data *****/

// current gear
int gear = 1;

/***** Inclinometer sensor data *****/

// current vehicle angle - horizontal reference
float curr_vehicle_angle = 0;

/***** Engine coolant sensor data *****/

#define ACCEL_FACTOR 0.1
#define AIR_COOL_FACTOR 0.05
#define MAX_TEMP_MOTOR 140
#define BASE_TEMP 80

// Current engine coolant temperature
float curr_cool_temp = 0;

/***** Battery sensor data *****/

// Current battery voltage
float bat_voltage = 12;

// Current battery State of Charge
float bat_soc = 80;

/***** AC sensor data *****/

// Temperature outside the vehicle
float out_temp = 28;

// Temperature inside the vehicle
float in_temp = 25;

// Internal temperature reference
float set_temp = 23;

/***** Door sensor data *****/

// Any door opened | 0 - no | 1 - yes |
int opened_door = 0;

/***** Calculate engine temperature *****/

float calculate_engine_temp(float velocidade, int rpm) {
    float temp_rise = rpm/10 * ACCEL_FACTOR;
    float cooling_effect = velocidade * AIR_COOL_FACTOR;
    float temp = BASE_TEMP + temp_rise - cooling_effect;
    return fminf(MAX_TEMP_MOTOR, temp);
}