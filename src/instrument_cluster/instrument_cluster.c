#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CAN_INTERFACE      ("vcan0")
#define PERMISSIONS        (0666)
#define ERROR_CODE         (1)
#define FIFO_PATH "/tmp/command_pipe"

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

int main(void) 
{
    int sock = -1;  
    sock = create_can_socket(CAN_INTERFACE);
    if (sock < 0)
    {
        return EXIT_FAILURE;
    }

    mkfifo(FIFO_PATH, PERMISSIONS);

    char input[AES_BLOCK_SIZE + 1];
    int fifo_fd;

    if (!init_logging_system()) {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    (void)printf("Waiting for new commands...\n");
    (void)fflush(stdout);

    for(;;) 
    {
        fifo_fd = open(FIFO_PATH, O_RDONLY);
        if (fifo_fd < 0) 
        {
            perror("Error opening FIFO");
            continue;
        }

        memset(input, 0, sizeof(input));
        read(fifo_fd, input, AES_BLOCK_SIZE);
        close(fifo_fd);
        
        // Remove any trailing newline
        input[strcspn(input, "\r\n")] = '\0';

        (void)printf("%s\n", input);
        (void)fflush(stdout);

        if (strcmp(input, "exit") == 0)
        {
            (void)printf("Exiting sender.\n");
            break;
        }
        if (strcmp(input, "") != 0) 
        {
            check_input_command(input, sock);
        }
    }

    close_can_socket(sock);
    unlink(FIFO_PATH);
    cleanup_logging_system();
    (void)printf("Sender terminated successfully.\n");
    return EXIT_SUCCESS;
}