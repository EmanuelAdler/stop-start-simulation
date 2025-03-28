#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include "instrument_cluster_func.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void check_input_command(char* option, int socket)
{
    if (strcmp(option, "press_start_stop") == 0)
    {
        send_encrypted_message(socket, option, CAN_ID_COMMAND);
        log_toggle_event("[INFO] System button toggled");
    }
    else if (strcmp(option, "show_dashboard") == 0)
    {
        send_encrypted_message(socket, option, CAN_ID_COMMAND);
    }
    else
    {
        printf("Invalid input. Try again.\n");
        fflush(stdout);
    }
}
