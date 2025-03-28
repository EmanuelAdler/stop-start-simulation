#include "instrument_cluster_func.h"

#define CAN_INTERFACE      ("vcan0")
#define PERMISSIONS        (0666)
#define ERROR_CODE         (1)
#define FIFO_PATH "/tmp/command_pipe"

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