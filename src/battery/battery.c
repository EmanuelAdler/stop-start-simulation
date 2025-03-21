#include "battery.h"
#include <stdio.h>
#include <pthread.h>

// External variables from simu_sensors.c
extern float bat_voltage;
extern float bat_soc;
extern pthread_mutex_t mutex_sensors;

// Initialize battery monitor
void battery_monitor_init(void) {
    printf("Battery monitor initialized.\n");
}

// Update battery monitor
void battery_monitor_update(void) {
    pthread_mutex_lock(&mutex_sensors);
    bool sufficient = (bat_voltage >= BATTERY_VOLTAGE_THRESHOLD) && 
                      (bat_soc >= BATTERY_SOC_THRESHOLD);
    pthread_mutex_unlock(&mutex_sensors);

    if (!sufficient) {
        printf("Warning: Battery level insufficient!\n");
    }
}

// Check if battery is sufficient
bool is_battery_sufficient(void) {
    pthread_mutex_lock(&mutex_sensors);
    bool sufficient = (bat_voltage >= BATTERY_VOLTAGE_THRESHOLD) && 
                      (bat_soc >= BATTERY_SOC_THRESHOLD);
    pthread_mutex_unlock(&mutex_sensors);
    return sufficient;
}

//baterry status
void log_battery_status(void) {
    pthread_mutex_lock(&mutex_sensors);
    printf("Battery Voltage: %.2fV\n", bat_voltage);
    printf("Battery SoC: %.2f%%\n", bat_soc);
    pthread_mutex_unlock(&mutex_sensors);
}