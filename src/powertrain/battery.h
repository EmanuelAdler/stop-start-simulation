#ifndef BATTERY_H
#define BATTERY_H

#include "powertrain.h"
#include "can_comms.h"
#include "logging.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

// Thresholds based on requirements
#define BATTERY_VOLTAGE_THRESHOLD 12.0
#define BATTERY_SOC_THRESHOLD 70        

// Battery Functions
void battery_monitor_init(void);
void battery_monitor_update(void);
bool is_battery_sufficient(void);
void log_battery_status(void);

#endif // BATTERY_H