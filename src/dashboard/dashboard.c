#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAN_INTERFACE      ("vcan0")
#define SUCCESS_CODE       (0)
#define ERROR_CODE         (1)

int main(void) 
{
    int sock = -1;
    sock = create_can_socket(CAN_INTERFACE);
    if (sock < 0)
    {
        return ERROR_CODE;
    }

    if (!init_logging_system()) {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    (void)printf("Listening for CAN frames...\n");
    (void)fflush(stdout);

    process_received_frame(sock);

    close_can_socket(sock);
    cleanup_logging_system();
    return SUCCESS_CODE;
}
