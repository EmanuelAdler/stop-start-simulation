#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// Thresholds based on requirements
#define BATTERY_VOLTAGE_THRESHOLD 12.0
#define BATTERY_SOC_THRESHOLD 70        

// External references to BCM battery variables
extern float batt_volt;
extern float batt_soc;
extern pthread_mutex_t mutex_bcm;

// Battery Functions
void battery_monitor_init(void);
void battery_monitor_update(void);
bool is_battery_sufficient(void);
void log_battery_status(void);

#endif // BATTERY_H