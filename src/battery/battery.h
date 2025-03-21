#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>
#include <stdbool.h>

// Thresholds based on requirements
#define BATTERY_VOLTAGE_THRESHOLD 12.2  // Minimum voltage under load (RSYS4.3)
#define BATTERY_SOC_THRESHOLD 70        // Minimum state of charge percentage (RSYS4.3)

// Battery Functions
void battery_monitor_init(void);
void battery_monitor_update(void);
bool is_battery_sufficient(void);
void log_battery_status(void);

#endif // BATTERY_H