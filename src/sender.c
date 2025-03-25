#include "common_includes/can_socket.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CAN_INTERFACE ("vcan0")
#define CAN_ID        (0x123U)
#define PERMISSIONS   (0666)
#define FIFO_PATH "/tmp/can_pipe"

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

    printf("Type a message to send (max 16 chars) or 'exit' to terminate:\n");

    for(;;) 
    {
        fifo_fd = open(FIFO_PATH, O_RDONLY);
        if (fifo_fd < 0) 
        {
            perror("Errror opening FIFO");
            continue;
        }

        memset(input, 0, sizeof(input));
        read(fifo_fd, input, AES_BLOCK_SIZE);
        close(fifo_fd);
        (void)printf("%s\n", input);
        (void)fflush(stdout);

        if (strcmp(input, "exit") == 0)
        {
            (void)printf("Exiting sender.\n");
            break;
        }
        if (strcmp(input, "") != 0) 
        {
            send_encrypted_message(sock, input, CAN_ID);
        }
    }

    close_can_socket(sock);
    unlink(FIFO_PATH);
    (void)printf("Sender terminated successfully.\n");
    return EXIT_SUCCESS;
}