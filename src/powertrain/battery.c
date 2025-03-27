#include "battery.h"
#include "logging.h"
#include <stdio.h>
#include <stdbool.h>

// Update battery monitor
void battery_monitor_update(void) {
    if (!is_battery_sufficient()) {
        pthread_mutex_lock(&mutex_bcm);
        char warning_msg[256];
        snprintf(warning_msg, sizeof(warning_msg), 
                 "Warning: Battery level insufficient! Voltage: %.2f V, State of Charge: %.2f%%", 
                 batt_volt, batt_soc);
        pthread_mutex_unlock(&mutex_bcm);
        
        log_toggle_event(warning_msg);
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
    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), 
             "Battery Status - Voltage: %.2f V, State of Charge: %.2f%%", 
             batt_volt, batt_soc);
    pthread_mutex_unlock(&mutex_bcm);
    
    log_toggle_event(status_msg);
}