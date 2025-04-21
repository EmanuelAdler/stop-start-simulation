#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include "dashboard_func.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAN_INTERFACE ("vcan0")
#define SUCCESS_CODE (0)
#define ERROR_CODE (1)

/* UI */

// Define the panel objects
ValuePanel *panel_dash = NULL;
ScrollPanel *panel_log = NULL;

int main(void)
{
    /* UI */

    if (!initscr())
    {
        fprintf(stderr, "Error initializing ncurses.\n");
        return ERROR_CODE;
    }
    cbreak();
    noecho();

    init_colors();

    // Create panels
    panel_dash = create_value_panel(SCROLL_PANEL_HEIGHT, VALUE_PANEL_WIDTH, 1, 1, "Dashboard");
    panel_log = create_log_panel(VALUE_PANEL_HEIGHT, SCROLL_PANEL_WIDTH, 1, MSG_LOG_PANEL_OFFSET, "Message Log");

    /* CAN communication */

    int sock = -1;
    sock = create_can_socket(CAN_INTERFACE);
    if (sock < 0)
    {
        return ERROR_CODE;
    }

    if (!init_logging_system())
    {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    add_to_log(panel_log, "Waiting CAN frames...");

    process_received_frame(sock);

    close_can_socket(sock);
    cleanup_logging_system();

    delwin(panel_log->win);
    delwin(panel_dash->win);

    endwin();

    return SUCCESS_CODE;
}
