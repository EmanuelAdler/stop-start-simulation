#include <ncurses.h>
#include <panel.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

// Color pair definitions
#define GREEN_TEXT  1
#define RED_TEXT    2
#define YELLOW_TEXT 3
#define NORMAL_TEXT 4

// Value panel

#define VALUE_PANEL_HEIGHT      20
#define VALUE_PANEL_WIDTH       40

#define VALUE_PRINT_COL 32

#define SPEED_ROW       1
#define TILT_ROW        2
#define IN_TEMP_ROW     3
#define EXT_TEMP_ROW    4
#define DOOR_ROW        5
#define ENGI_TEMP_ROW   6
#define BATT_VOLT_ROW   7
#define BATT_SOC_ROW    8
#define ACCEL_ROW       10
#define BRAKE_ROW       11
#define GEAR_ROW        12

#define SYSTEM_ST_ROW   14
#define ENGINE_ST_ROW   15

#define NUM_SYS_ACTIV   18

typedef struct {
    WINDOW *win;
    int height;
    int width;
} ValuePanel;

// Scroll panel

#define SCROLL_PANEL_HEIGHT     20
#define SCROLL_PANEL_WIDTH      46
#define BORDER_MARGIN           2
#define MAX_LOG_LINES           18
#define TMSTMP_SIZE             10

#define MAX_MSG_WIDTH           44

typedef struct {
    WINDOW *win;
    int height;
    int width;
    int line_count;
} ScrollPanel;

typedef struct { 
    int height;
    int width; 
} Size;

typedef struct { 
    int y_cord;
    int x_cord; 
} Position;

// Declare extern panels to be defined in "dashboard.c"
extern ValuePanel *panel_dash;
extern ScrollPanel *panel_log;

void init_colors();

ScrollPanel *create_log_panel(Size siz, Position pos, const char *title);

void add_to_log(ScrollPanel *panel, const char *text);

ValuePanel *create_value_panel(Size siz, Position pos, const char *title);

void update_value_panel(ValuePanel *panel, int row, const char *value, int color_pair);

void destroy_panel(WINDOW *win);
