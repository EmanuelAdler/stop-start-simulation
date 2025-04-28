#include "panels.h"

// Static allocation for panels
static ScrollPanel log_panel_storage;
static ValuePanel value_panel_storage;

// Fixed buffer for storing lines
static char line_buffer[MAX_LOG_LINES][MAX_MSG_WIDTH];
static int buffer_index = 0;

/* // Sleep parameters
#define MICRO_CONSTANT_CONV (1000000L)
#define NANO_CONSTANT_CONV (1000)

// Sleep for a given number of microseconds
void sleep_microseconds(long int microseconds)
{
    struct timespec tsc;
    tsc.tv_sec = microseconds / MICRO_CONSTANT_CONV;
    tsc.tv_nsec = (microseconds % MICRO_CONSTANT_CONV) * NANO_CONSTANT_CONV;
    nanosleep(&tsc, NULL);
} */

// Color function
void init_colors()
{
    if (has_colors() == FALSE)
    {
        return;
    }

    start_color();

    init_pair(GREEN_TEXT, COLOR_GREEN, COLOR_BLACK);
    init_pair(RED_TEXT, COLOR_RED, COLOR_BLACK);
    init_pair(YELLOW_TEXT, COLOR_YELLOW, COLOR_BLACK);
    init_pair(NORMAL_TEXT, COLOR_WHITE, COLOR_BLACK);
}

/* Log panel functions */

void add_to_log(ScrollPanel *panel, const char *text)
{
    if (panel == NULL || panel->win == NULL || text == NULL)
    {
        return;
    }

    // Get timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[TMSTMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);

    // Calculate inner dimensions
    int inner_height = panel->height - 2;
    int inner_width = panel->width - 2;

    // Format the log entry
    char formatted_text[MAX_MSG_WIDTH];
    snprintf(formatted_text, sizeof(formatted_text), "[%s] %s", timestamp, text);

    // Store in circular buffer
    strncpy(line_buffer[buffer_index], formatted_text, MAX_MSG_WIDTH - 1);
    line_buffer[buffer_index][MAX_MSG_WIDTH - 1] = '\0';
    buffer_index = (buffer_index + 1) % MAX_LOG_LINES;

    // Update line count (capped at MAX_LOG_LINES)
    if (panel->line_count < MAX_LOG_LINES)
    {
        panel->line_count++;
    }

    // Create content window
    WINDOW *content = derwin(panel->win, inner_height, inner_width, 1, 1);
    if (content == NULL)
    {
        return;
    }

    // Clear the content window
    werase(content);

    // Determine how many lines we can actually display
    int lines_to_display = (panel->line_count < inner_height) ? panel->line_count : inner_height;

    // Calculate the starting index in the circular buffer
    int start_index;
    if (panel->line_count <= inner_height)
    {
        start_index = 0;
    }
    else
    {
        start_index = (buffer_index - lines_to_display + MAX_LOG_LINES) % MAX_LOG_LINES;
    }

    // Display the lines
    for (int i = 0; i < lines_to_display; i++)
    {
        int buf_index = (start_index + i) % MAX_LOG_LINES;
        mvwprintw(content, i, 0, "%-*.*s", inner_width, inner_width, line_buffer[buf_index]);
    }

    // Refresh and clean up
    wrefresh(content);
    delwin(content);
}

ScrollPanel *create_log_panel(Size siz, Position pos, const char *title)
{
    ScrollPanel *panel = &log_panel_storage;

    panel->win = newwin(siz.height, siz.width, pos.y_cord, pos.x_cord);
    if (!panel->win)
    {
        return NULL;
    }

    panel->height = siz.height;
    panel->width = siz.width;
    panel->line_count = 0;

    box(panel->win, 0, 0);
    if (title)
    {
        int max_title_width = siz.width - 4;
        int title_len = (int)strlen(title);
        int pos = 2;

        wattron(panel->win, A_BOLD);
        if (title_len > max_title_width)
        {
            mvwprintw(panel->win, 0, pos, " %.*s ", max_title_width, title);
        }
        else
        {
            pos = (siz.width - title_len) / 2;
            mvwprintw(panel->win, 0, pos, " %s ", title);
        }
        wattroff(panel->win, A_BOLD);
    }

    scrollok(panel->win, TRUE);
    wrefresh(panel->win);
    return panel;
}

/* Value panel functions */

ValuePanel *create_value_panel(Size siz, Position pos, const char *title)
{
    ValuePanel *panel = &value_panel_storage;

    // Create main window with borders
    panel->win = newwin(siz.height, siz.width, pos.y_cord, pos.x_cord);
    panel->height = siz.height;
    panel->width = siz.width;

    // Set background
    wbkgd(panel->win, COLOR_PAIR(NORMAL_TEXT));

    // Draw border and title (only once)
    box(panel->win, 0, 0);
    wattron(panel->win, A_BOLD);
    mvwprintw(panel->win, 0, (int)(siz.width - strlen(title) - 4) / 2, " %s ", title);
    wattroff(panel->win, A_BOLD);

    // Draw all static content offset by 1 (inside borders)
    wattron(panel->win, COLOR_PAIR(NORMAL_TEXT));

    mvwprintw(panel->win, SPEED_ROW, 1, "Speed (Km/h):");
    mvwprintw(panel->win, TILT_ROW, 1, "Tilt Angle (°):");
    mvwprintw(panel->win, IN_TEMP_ROW, 1, "Internal Temp (°C):");
    mvwprintw(panel->win, EXT_TEMP_ROW, 1, "External Temp (°C):");
    mvwprintw(panel->win, DOOR_ROW, 1, "Door Open:");
    mvwprintw(panel->win, ENGI_TEMP_ROW, 1, "Engine Temp (°C):");
    mvwprintw(panel->win, BATT_VOLT_ROW, 1, "Battery Voltage (V):");
    mvwprintw(panel->win, BATT_SOC_ROW, 1, "Battery SoC (%%):");
    mvwprintw(panel->win, ACCEL_ROW, 1, "Accelerator:");
    mvwprintw(panel->win, BRAKE_ROW, 1, "Brake:");
    mvwprintw(panel->win, GEAR_ROW, 1, "Gear:");

    mvwprintw(panel->win, SYSTEM_ST_ROW, 1, "Stop/Start System:");
    mvwprintw(panel->win, ENGINE_ST_ROW, 1, "Engine Status:");

    mvwprintw(panel->win, NUM_SYS_ACTIV, 1, "Number of deactivations:");

    // Initialize values (using update function but with coordinates inside border)
    update_value_panel(panel, SPEED_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, TILT_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, IN_TEMP_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, EXT_TEMP_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, DOOR_ROW, "No", NORMAL_TEXT);
    update_value_panel(panel, ENGI_TEMP_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, BATT_VOLT_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, BATT_SOC_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, ACCEL_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, BRAKE_ROW, "-", NORMAL_TEXT);
    update_value_panel(panel, GEAR_ROW, "-", NORMAL_TEXT);

    update_value_panel(panel, SYSTEM_ST_ROW, "OFF", RED_TEXT);
    update_value_panel(panel, ENGINE_ST_ROW, "OFF", RED_TEXT);

    update_value_panel(panel, NUM_SYS_ACTIV, "0", NORMAL_TEXT);

    wrefresh(panel->win);
    return panel;
}

void update_value_panel(ValuePanel *panel, int row, const char *value, int color_pair)
{
    int value_col = VALUE_PRINT_COL;
    int max_width = panel->width - value_col - 2; // -2 for borders

    // Ensure we're writing inside the border (y: row, x: 1 to width-2)
    int write_col = MAX(1, value_col);            // At least 1 to stay inside left border
    write_col = MIN(write_col, panel->width - 2); // At most width-2 to stay inside right border

    // Move to position inside border
    wmove(panel->win, row, write_col);

    // Clear to end of line but stay inside right border
    int clear_width = panel->width - write_col - 1;
    for (int i = 0; i < clear_width; i++)
    {
        waddch(panel->win, ' ');
    }

    // Move back to write position
    wmove(panel->win, row, write_col);

    wattron(panel->win, COLOR_PAIR(color_pair));

    if (strlen(value) > (size_t)max_width)
    {
        char buf[max_width + 1];
        strncpy(buf, value, max_width);
        buf[max_width] = '\0';
        wprintw(panel->win, "%s", buf);
    }
    else
    {
        wprintw(panel->win, "%s", value);
    }

    wattroff(panel->win, COLOR_PAIR(color_pair));

    // No need to redraw the right border since we stayed inside
    wrefresh(panel->win);
}

// Panel destroy function
void destroy_panel(WINDOW *win)
{
    delwin(win);
}