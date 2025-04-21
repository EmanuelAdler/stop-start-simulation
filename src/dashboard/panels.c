#include "panels.h"

// Static allocation for panels
static ScrollPanel log_panel_storage;
static ValuePanel value_panel_storage;

// Fixed buffer for storing lines
static char line_buffer[MAX_LOG_LINES][MAX_MSG_WIDTH];
static int buffer_index = 0;

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

// Log panel functions

void add_to_log(ScrollPanel *panel, const char *text)
{
    if (panel == NULL || panel->win == NULL || text == NULL)
    {
        return;
    }

    /* Get current time for timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[TMSTMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);

    /* Calculate inner dimensions */
    int inner_height = panel->height - 2;
    int inner_width = panel->width - 2;

    /* Create inner window for content - using full width */
    WINDOW *content = derwin(panel->win, inner_height, inner_width, 1, 1);
    if (content == NULL)
    {
        return;
    }

    /* Format text with timestamp */
    char formatted_text[MAX_MSG_WIDTH];
    snprintf(formatted_text, sizeof(formatted_text), "[%s] %s", timestamp, text);

    /* Store the text in our fixed buffer */
    strncpy(line_buffer[buffer_index], formatted_text, MAX_MSG_WIDTH - 1);
    line_buffer[buffer_index][MAX_MSG_WIDTH - 1] = '\0';
    buffer_index = (buffer_index + 1) % MAX_LOG_LINES;
    if (panel->line_count < MAX_LOG_LINES)
    {
        panel->line_count++;
    }

    /* Clear and redraw all visible content */
    werase(content);
    scrollok(content, TRUE);

    /* Determine start position for circular buffer */
    int start = (buffer_index - inner_height) % MAX_LOG_LINES;
    if (start < 0)
    {
        start += MAX_LOG_LINES;
    }

    /* Display the appropriate lines */
    int lines_to_show = (panel->line_count < inner_height) ? panel->line_count : inner_height;
    for (int i = 0; i < lines_to_show; i++)
    {
        int idx = (start + i) % MAX_LOG_LINES;
        mvwprintw(content, i, 0, "%-*.*s", inner_width, inner_width, line_buffer[idx]);
    }

    /* Auto-scroll if needed */
    if (panel->line_count > inner_height)
    {
        wscrl(content, panel->line_count - inner_height);
    }

    wrefresh(content);
    delwin(content);
}

ScrollPanel *create_log_panel(int height, int width, int y_coord, int x_coord, const char *title)
{
    ScrollPanel *panel = &log_panel_storage;

    panel->win = newwin(height, width, y_coord, x_coord);
    if (!panel->win)
    {
        return NULL;
    }

    panel->height = height;
    panel->width = width;
    panel->line_count = 0;

    box(panel->win, 0, 0);
    if (title)
    {
        int max_title_width = width - 4;
        int title_len = (int)strlen(title);
        int pos = 2;

        wattron(panel->win, A_BOLD);
        if (title_len > max_title_width)
        {
            mvwprintw(panel->win, 0, pos, " %.*s ", max_title_width, title);
        }
        else
        {
            pos = (width - title_len) / 2;
            mvwprintw(panel->win, 0, pos, " %s ", title);
        }
        wattroff(panel->win, A_BOLD);
    }

    scrollok(panel->win, TRUE);
    wrefresh(panel->win);
    return panel;
}

// Value panel functions
ValuePanel *create_value_panel(int height, int width, int y_cord, int x_cord, const char *title)
{
    ValuePanel *panel = &value_panel_storage;

    panel->win = newwin(height, width, y_cord, x_cord);
    panel->height = height;
    panel->width = width;

    wbkgd(panel->win, COLOR_PAIR(NORMAL_TEXT));

    box(panel->win, 0, 0);
    wattron(panel->win, A_BOLD);
    mvwprintw(panel->win, 0, (int)(width - strlen(title) - 4) / 2, " %s ", title);
    wattroff(panel->win, A_BOLD);

    wattron(panel->win, COLOR_PAIR(NORMAL_TEXT));

    mvwprintw(panel->win, SPEED_ROW, 1, "Speed (Km/h):");
    mvwprintw(panel->win, TILT_ROW, 1, "Tilt Angle (°):");
    mvwprintw(panel->win, IN_TEMP_ROW, 1, "Internal Temp (°C):");
    mvwprintw(panel->win, EXT_TEMP_ROW, 1, "External Temp (°C):");
    mvwprintw(panel->win, DOOR_ROW, 1, "Door Open:");
    mvwprintw(panel->win, ENGI_TEMP_ROW, 1, "Engine Temp. (°C):");
    mvwprintw(panel->win, BATT_VOLT_ROW, 1, "Battery Voltage (V):");
    mvwprintw(panel->win, BATT_SOC_ROW, 1, "Battery Soc (%%):");
    mvwprintw(panel->win, ACCEL_ROW, 1, "Accelerator:");
    mvwprintw(panel->win, BRAKE_ROW, 1, "Brake:");
    mvwprintw(panel->win, GEAR_ROW, 1, "Gear:");

    mvwprintw(panel->win, SYSTEM_ST_ROW, 1, "Stop/Start System:");
    mvwprintw(panel->win, ENGINE_ST_ROW, 1, "Engine Status:");

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

    update_value_panel(panel, SYSTEM_ST_ROW, "Off", RED_TEXT);
    update_value_panel(panel, ENGINE_ST_ROW, "Off", RED_TEXT);

    wrefresh(panel->win);
    return panel;
}

// Update value panel
void update_value_panel(ValuePanel *panel, int row, const char *value, int color_pair)
{
    int value_col = VALUE_PRINT_COL;
    int max_width = panel->width - value_col - 2;

    wmove(panel->win, row, value_col);
    wclrtoeol(panel->win);

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
    mvwaddch(panel->win, row, panel->width - 1, ACS_VLINE);
    wrefresh(panel->win);
}

// Panel destroy function
void destroy_panel(ScrollPanel *panel)
{
    delwin(panel->win);
}