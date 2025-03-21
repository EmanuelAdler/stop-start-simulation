#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

// Initialize logging system
bool init_logging_system(void);

// Log a toggle event with a timestamp
void log_toggle_event(char* message);

// Cleanup logging system
void cleanup_logging_system(void);

#endif // LOGGING_H
