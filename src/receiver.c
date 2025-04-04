#include "common_includes/can_socket.h"
#include "common_includes/logging.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAN_INTERFACE      ("vcan0")
#define CAN_DATA_LENGTH    (8)
#define LOG_MESSAGE_SIZE   (50)
#define SUCCESS_CODE       (0)
#define ERROR_CODE         (1)

void process_received_frame(int sock) 
{
    struct can_frame frame;
    unsigned char encrypted_data[AES_BLOCK_SIZE];
    char decrypted_message[AES_BLOCK_SIZE];
    int received_bytes = 0;
    char message_log[LOG_MESSAGE_SIZE];

    for(;;)
    {
        if (receive_can_frame(sock, &frame) == 0) 
        {
            (void)printf("Received CAN ID: %X Data: ", frame.can_id);
            for (int i = 0; i < frame.can_dlc; i++)
            {
                (void)printf("%02X ", frame.data[i]);
            }
            (void)printf("\n");
            (void)fflush(stdout);

            if (frame.can_dlc == CAN_DATA_LENGTH) 
            {
                memcpy(encrypted_data + received_bytes, frame.data, CAN_DATA_LENGTH);
                received_bytes += CAN_DATA_LENGTH;

                if (received_bytes == AES_BLOCK_SIZE) 
                {
                    decrypt_data(encrypted_data, decrypted_message, received_bytes);
                    (void)printf("Decrypted message: %s\n", decrypted_message);
                    (void)fflush(stdout);
                    snprintf(message_log, sizeof(message_log), "Message received via CAN: %s", decrypted_message);
                    log_toggle_event(message_log);
                    received_bytes = 0;
                }
            } 
            else 
            {
                (void)printf("⚠️ Warning: Unexpected frame size (%d bytes). Ignoring.\n", frame.can_dlc);
            }
        }
    }
}

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