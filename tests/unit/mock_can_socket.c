#include "../../src/common_includes/can_socket.h"
#include <stdio.h>

#define LAST_MESSAGE_SIZE (256)

static int s_send_count = 0;
static int s_received_count = 0;
static char s_last_message_sent[LAST_MESSAGE_SIZE] = {0};

/* 
 * Fake version of send_encrypted_message.
 * - Do not open real socket
 * - Simply log the call so we can validate it later. 
 */
void send_encrypted_message(int sock, const char *message, int can_id)
{
    (void) sock;
    (void) can_id;

    s_send_count++;
    snprintf(s_last_message_sent, sizeof(s_last_message_sent), "%s", message);
    printf("[STUB] send_encrypted_message('%s')\n", message);
}

/* 
 * Fake version of receive_can_frame.
 * - Do not open real socket
 * - Simply log the call so we can validate it later. 
 */
int receive_can_frame(int sock, struct can_frame *frame)
{
    (void)sock;
    (void)frame;

    printf("[MOCK] receive_can_frame called\n");
    s_received_count++;
    return 0;  // or -1 to simulate an error
}

void decrypt_data(const unsigned char *input, char *output, int input_len)
{
    (void)input_len;

    snprintf(output, input_len, "%s", input);
}

int stub_can_get_send_count(void)
{
    return s_send_count;
}

int stub_can_get_received_count(void)
{
    return s_received_count;
}

const char* stub_can_get_last_message(void)
{
    return s_last_message_sent;
}

void stub_can_reset(void)
{
    s_send_count = 0;
    s_received_count = 0;
    s_last_message_sent[0] = '\0';
}
