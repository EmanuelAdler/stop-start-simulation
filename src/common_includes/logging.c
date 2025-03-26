#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define LOG_FILE_PATH "/home/paulo/app/logs/diagnostics.log"
#define TIME_STR_SIZE (64)

static FILE *logFile = NULL;

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

bool init_logging_system(void) {
    logFile = fopen(LOG_FILE_PATH, "a");
    return (logFile != NULL);
}

void log_toggle_event(char* message) {
    if (logFile == NULL) {
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

void cleanup_logging_system(void) {
    if (logFile) {
        fclose(logFile);
        logFile = NULL;
    }
}
