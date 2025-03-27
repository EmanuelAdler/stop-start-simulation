#include "battery.h"
#include <stdio.h>

// Initialize battery monitor
void battery_monitor_init(void) {
    printf("Battery monitor initialized.\n");
}

// Update battery monitor
void battery_monitor_update(void) {
    pthread_mutex_lock(&mutex_bcm);
    bool sufficient = (batt_volt >= BATTERY_VOLTAGE_THRESHOLD) && 
                      (batt_soc >= BATTERY_SOC_THRESHOLD);
    pthread_mutex_unlock(&mutex_bcm);

    if (!sufficient) {
        printf("Warning: Battery level insufficient!\n");
        printf("Voltage: %.2f V, State of Charge: %.2f%%\n", batt_volt, batt_soc);
    }
}

// Check if battery is sufficient
bool is_battery_sufficient(void) {
    pthread_mutex_lock(&mutex_bcm);
    bool sufficient = (batt_volt >= BATTERY_VOLTAGE_THRESHOLD) && 
                      (batt_soc >= BATTERY_SOC_THRESHOLD);
    pthread_mutex_unlock(&mutex_bcm);
    return sufficient;
}

// Log battery status
void log_battery_status(void) {
    pthread_mutex_lock(&mutex_bcm);
    printf("Battery Voltage: %.2f V\n", batt_volt);
    printf("Battery State of Charge: %.2f%%\n", batt_soc);
    pthread_mutex_unlock(&mutex_bcm);
}