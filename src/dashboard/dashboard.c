#include "dashboard_func.h"

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
    curs_set(0);

    init_colors();

    // Create panels
    panel_dash = create_value_panel(
        (Size){SCROLL_PANEL_HEIGHT, VALUE_PANEL_WIDTH}, 
        (Position){1, 1}, 
        "Dashboard"
    );
    panel_log = create_log_panel(
        (Size){VALUE_PANEL_HEIGHT, SCROLL_PANEL_WIDTH}, 
        (Position){1, MSG_LOG_PANEL_OFFSET}, 
        "Message Log"
    );

    if (!init_logging_system())
    {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    /* CAN communication */

    sock_dash = -1;
    sock_dash = create_can_socket(CAN_INTERFACE);
    if (sock_dash < 0)
    {
        return ERROR_CODE;
    }

    add_to_log(panel_log, "Waiting CAN frames...");

    /* Create CAN thread */
    pthread_t can_thread;
    pthread_t parse_thread;

    init_can_buffer();

    if (pthread_create(&can_thread, NULL, can_receiver_thread, NULL) != 0) {
        add_to_log(panel_log, "ERROR: Thread creation failed");
        close_can_socket(sock_dash);
        return ERROR_CODE;
    }

    if (pthread_create(&parse_thread, NULL, process_frame_thread, NULL) != 0) {
        add_to_log(panel_log, "ERROR: Thread creation failed");
        close_can_socket(sock_dash);
        return ERROR_CODE;
    }

    pthread_join(can_thread, NULL);
    pthread_join(parse_thread, NULL);

    /* Cleanup */
    pthread_cancel(can_thread);
    pthread_cancel(parse_thread);
    
    close_can_socket(sock_dash);
    cleanup_can_buffer();
    
    /* UI cleanup */
    destroy_panel(panel_log->win);
    destroy_panel(panel_dash->win);
    cleanup_logging_system();
    endwin();
}
