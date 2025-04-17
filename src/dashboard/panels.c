#include "panels.h"

// Color function

void init_colors()
{
    // Check if terminal supports color
    if (has_colors() == FALSE)
    {
        return;
    }

    start_color();

    // Define color pairs (foreground, background)
    init_pair(GREEN_TEXT, COLOR_GREEN, COLOR_BLACK);
    init_pair(RED_TEXT, COLOR_RED, COLOR_BLACK);
    init_pair(YELLOW_TEXT, COLOR_YELLOW, COLOR_BLACK);
    init_pair(NORMAL_TEXT, COLOR_WHITE, COLOR_BLACK);
}

// Log panel functions

void add_to_log(ScrollPanel *panel, const char *text)
{
    // Get timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", t);

    // Calculate available width (accounting for borders and timestamp)
    int available_width = panel->width - 4; // 1 char border each side + space

    // Create display line (truncate if too long)
    char display[SCROLL_PANEL_MAX_LINE_LENGTH + 20];
    snprintf(display, sizeof(display), "[%s] %.*s",
             timestamp, available_width - 12, text); // 12 chars for timestamp format

    // Get current cursor position
    int y, x;
    getyx(panel->win, y, x);

    // If at bottom, scroll first
    if (y >= panel->height - 2)
    {
        wscrl(panel->win, 1);
        y = panel->height - 2; // Stay above bottom border
    }
    else
    {
        y++; // Move to next line
    }

    // Write text inside borders (1,1 is inside the top-left border)
    mvwprintw(panel->win, y, 1, "%s", display);
    panel->line_count++;

    wrefresh(panel->win);
}

ScrollPanel *create_titled_scroll_panel(int height, int width, int y, int x, const char *title)
{
    ScrollPanel *panel = malloc(sizeof(ScrollPanel));
    panel->win = newwin(height, width, y, x);

    // Draw box with title
    box(panel->win, 0, 0);
    mvwprintw(panel->win, 0, (width - strlen(title) - 4) / 2, " %s ", title);

    panel->height = height;
    panel->width = width;
    panel->line_count = 0;

    scrollok(panel->win, TRUE);
    wrefresh(panel->win);
    return panel;
}

// Value panel functions

ValuePanel *create_value_panel(int height, int width, int y, int x, const char *title)
{
    ValuePanel *panel = malloc(sizeof(ValuePanel));
    panel->win = newwin(height, width, y, x);
    panel->height = height;
    panel->width = width;

    // Enable color in this window
    wbkgd(panel->win, COLOR_PAIR(NORMAL_TEXT));

    // Draw box with centered title
    box(panel->win, 0, 0);
    wattron(panel->win, A_BOLD);
    mvwprintw(panel->win, 0, (width - strlen(title) - 4) / 2, " %s ", title);
    wattroff(panel->win, A_BOLD);

    // Print labels
    wattron(panel->win, COLOR_PAIR(NORMAL_TEXT));

    // Print labels with proper spacing
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

    // Initialize values with proper right padding
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

void update_value_panel(ValuePanel *panel, int row, const char *value, int color_pair)
{
    // Fixed value column position (right after labels)
    int value_col = 32; // Adjust based on your longest label

    // Calculate max available space
    int max_width = panel->width - value_col - 2; // -2 for right border

    // Move to position and clear the line
    wmove(panel->win, row, value_col);
    wclrtoeol(panel->win);

    // Apply color
    wattron(panel->win, COLOR_PAIR(color_pair));

    // Print truncated value if needed
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

    // Redraw right border if needed
    mvwaddch(panel->win, row, panel->width - 1, ACS_VLINE);

    wrefresh(panel->win);
}

void destroy_panel(ScrollPanel *panel)
{
    delwin(panel->win);
    free(panel);
}