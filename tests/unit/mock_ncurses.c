#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "../../src/dashboard/panels.h"

// Define the panel objects
ValuePanel *panel_dash = NULL;
ScrollPanel *panel_log = NULL;

#define MAX_MSG_SIZE 256
#define TMSTMP_SIZE 10
#define MAX_LOG_LINES 18

// Mock state
static struct
{
    // Log panel tracking
    char last_log_message[MAX_MSG_SIZE];
    int log_panel_call_count;

    // Value panel tracking
    char last_value_update[MAX_MSG_SIZE];
    int last_value_row;
    int last_color_pair;

    // Window management
    int windows_created;
    int windows_destroyed;
} mock_state;

// Mock implementations of panel functions
void add_to_log(ScrollPanel *panel, const char *text)
{
    if (!panel || !text)
    {
        return;
    }

    // Simulate timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[TMSTMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);

    // Store the formatted message
    snprintf(mock_state.last_log_message, sizeof(mock_state.last_log_message),
             "[%s] %s", timestamp, text);

    mock_state.log_panel_call_count++;
    if (panel->line_count < MAX_LOG_LINES)
    {
        panel->line_count++;
    }
}

ScrollPanel *create_log_panel(int height, int width, int y_cord, int x_cord, const char *title)
{
    static ScrollPanel panel;
    panel.height = height;
    panel.width = width;
    panel.line_count = 0;
    panel.win = (WINDOW *)0x1; // Dummy pointer

    mock_state.windows_created++;
    (void)y_cord;
    (void)x_cord;
    (void)title; // Silence unused parameter warnings
    return &panel;
}

ValuePanel *create_value_panel(int height, int width, int y_cord, int x_cord, const char *title)
{
    static ValuePanel panel;
    panel.height = height;
    panel.width = width;
    panel.win = (WINDOW *)0x1;

    mock_state.windows_created++;
    (void)y_cord;
    (void)x_cord;
    (void)title; // Silence unused parameter warnings
    return &panel;
}

void update_value_panel(ValuePanel *panel, int row, const char *value, int color_pair)
{
    if (!panel || !value)
    {
        return;
    }

    strncpy(mock_state.last_value_update, value, sizeof(mock_state.last_value_update));
    mock_state.last_value_row = row;
    mock_state.last_color_pair = color_pair;
}

void destroy_panel(WINDOW *win)
{
    mock_state.windows_destroyed++;
    (void)win; // Silence unused parameter warning
}

// Test helper functions
const char *mock_get_last_log_message(void) { return mock_state.last_log_message; }
int mock_get_log_call_count(void) { return mock_state.log_panel_call_count; }
const char *mock_get_last_value_update(void) { return mock_state.last_value_update; }
int mock_get_last_value_row(void) { return mock_state.last_value_row; }
int mock_get_last_color_pair(void) { return mock_state.last_color_pair; }
int mock_get_windows_created(void) { return mock_state.windows_created; }

void mock_reset_state(void)
{
    memset(&mock_state, 0, sizeof(mock_state));
}