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
    // Timestamp setup
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char timestamp[TMSTMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &tm_info);
    // The time shown depends on docker container time settings

    // Clean text
    char clean_text[MAX_MSG_WIDTH];
    int data_index = 0;
    for (int i = 0; text[i] && data_index < MAX_MSG_WIDTH - 1; i++)
    {
        if (isprint(text[i]))
        {
            clean_text[data_index++] = text[i];
        }
    }
    clean_text[data_index] = '\0';

    int content_width = panel->width - 2;
    char line[content_width + 1];

    snprintf(line, sizeof(line), "%s | %.*s", timestamp,
             content_width - TMSTMP_SIZE + 1,
             clean_text);

    if (panel->line_count >= panel->height - 2)
    {
        wscrl(panel->win, 1);
    }
    else
    {
        panel->line_count++;
    }

    // Clear the line fully before printing new content
    wmove(panel->win, panel->line_count, 1);
    wclrtoeol(panel->win); // this ensures there's no leftover text!
    mvwprintw(panel->win, panel->line_count, 1, "%.*s", content_width, line);

    box(panel->win, 0, 0);
    wrefresh(panel->win);
}

ScrollPanel *create_log_panel(int height, int width, int y_coord, int x_coord, const char *title)
{
    ScrollPanel *panel = malloc(sizeof(ScrollPanel));
    if (!panel){
        return NULL;
    }

    panel->win = newwin(height, width, y_coord, x_coord);
    if (!panel->win)
    {
        free(panel);
        return NULL;
    }

    // Initialize properties
    panel->height = height;
    panel->width = width;
    panel->line_count = 0;

    // Draw border with simple title (matches your dashboard style)
    box(panel->win, 0, 0);
    if (title)
    {
        int max_title_width = width - 4; // Account for border and padding
        int title_len = (int)strlen(title);

        // Center the title with truncation if needed
        int pos = 2;
        if (title_len > max_title_width)
        {
            mvwprintw(panel->win, 0, pos, " %.*s ", max_title_width, title);
        }
        else
        {
            pos = (width - title_len) / 2;
            mvwprintw(panel->win, 0, pos, " %s ", title);
        }
    }

    scrollok(panel->win, TRUE);
    wrefresh(panel->win);
    return panel;
}

// Value panel functions

ValuePanel *create_value_panel(int height, int width, int y_cord, int x_cord, const char *title)
{
    ValuePanel *panel = malloc(sizeof(ValuePanel));
    panel->win = newwin(height, width, y_cord, x_cord);
    panel->height = height;
    panel->width = width;

    // Enable color in this window
    wbkgd(panel->win, COLOR_PAIR(NORMAL_TEXT));

    // Draw box with centered title
    box(panel->win, 0, 0);
    wattron(panel->win, A_BOLD);
    mvwprintw(panel->win, 0, (int)(width - strlen(title) - 4) / 2, " %s ", title);
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
    int value_col = VALUE_PRINT_COL; // Adjust based on your longest label

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