#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define TIME_STR_SIZE (64)

static const char *log_file_path = "/app/logs/diagnostics.log";

static FILE *logFile = NULL;

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

bool init_logging_system(void)
{
    logFile = fopen(log_file_path, "a");
    return (logFile != NULL);
}

void set_log_file_path(const char *new_path)
{
    log_file_path = new_path;
}

/**
 * @brief Log events in a file.
 * @requirement SWR1.5
 */
void log_toggle_event(char* message)
{
    if (logFile == NULL)
    {
        return;
    }

    pthread_mutex_lock(&logMutex);

    // Get current time
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);

    char timeStr[TIME_STR_SIZE];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Write the event
    fprintf(logFile, "[%s] %s\n", timeStr, message);
    fflush(logFile);

    pthread_mutex_unlock(&logMutex);
}

void cleanup_logging_system(void)
{
    if (logFile)
    {
        fclose(logFile);
        logFile = NULL;
    }
}
