#include <ncurses.h>
#include <panel.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

// Color pair definitions
#define GREEN_TEXT 1
#define RED_TEXT 2
#define YELLOW_TEXT 3
#define NORMAL_TEXT 4

// Value panel

#define VALUE_PANEL_HEIGHT 20
#define VALUE_PANEL_WIDTH 40
#define VALUE_PANEL_NUM_ROWS 6

#define SPEED_ROW       1
#define TILT_ROW        2
#define IN_TEMP_ROW     3
#define EXT_TEMP_ROW    4
#define DOOR_ROW        5
#define ENGI_TEMP_ROW   6
#define BATT_VOLT_ROW   7
#define BATT_SOC_ROW    8
#define ACCEL_ROW       9
#define BRAKE_ROW       10
#define GEAR_ROW        11

#define SYSTEM_ST_ROW   16
#define ENGINE_ST_ROW   17

typedef struct {
    WINDOW *win;
    int height;
    int width;
} ValuePanel;

// Scroll panel

#define SCROLL_PANEL_HEIGHT 20
#define SCROLL_PANEL_WIDTH 40
#define SCROLL_PANEL_MAX_LINE_LENGTH 256

typedef struct {
    WINDOW *win;
    int height;
    int width;
    int line_count;
} ScrollPanel;

// Declare extern panels to be defined in "dashboard.c"
extern ValuePanel *panel_dash;
extern ScrollPanel *panel_log;

void init_colors();

ScrollPanel* create_titled_scroll_panel(int height, int width, int y, int x, const char *title);

void add_to_log(ScrollPanel *panel, const char *text);

ValuePanel* create_value_panel(int height, int width, int y, int x, const char *title);

void update_value_panel(ValuePanel *panel, int row, const char *value, int color_pair);

void destroy_panel(ScrollPanel *panel);
