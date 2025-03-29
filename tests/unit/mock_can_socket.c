#include "../../src/common_includes/can_id_list.h"
#include "../../src/common_includes/can_socket.h"
#include <stdio.h>
#include <string.h>
#include <linux/can.h>

#define LAST_MESSAGE_SIZE      (256)
#define CAN_DLC_CORRECT        (8)
#define CAN_DLC_INCORRECT      (5)
#define FIRST_CALL_BYTES       (0x11)
#define SECOND_CALL_BYTES      (0x22)
#define THIRD_CALL_BYTES       (0x33)

static int s_send_count = 0;
static int s_received_count = 0;
static int g_call_count = 0;
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
    switch (g_call_count)
    {
        case 0:
            // 1st call: valid CAN_ID_COMMAND, dlc=8
            frame->can_id  = CAN_ID_COMMAND;
            frame->can_dlc = CAN_DLC_CORRECT;
            memset(frame->data, FIRST_CALL_BYTES, CAN_DLC_CORRECT);
            break;
        case 1:
            // 2nd call: again dlc=8 -> triggers decrypt after 2 blocks
            frame->can_id  = CAN_ID_COMMAND;
            frame->can_dlc = CAN_DLC_CORRECT;
            memset(frame->data, SECOND_CALL_BYTES, CAN_DLC_CORRECT);
            break;
        case 2:
            // 3rd call: unexpected size -> triggers "Warning: Unexpected frame size"
            frame->can_id  = CAN_ID_COMMAND;
            frame->can_dlc = CAN_DLC_INCORRECT;
            memset(frame->data, THIRD_CALL_BYTES, CAN_DLC_INCORRECT);
            break;
        default:
            // Return -1 on subsequent calls -> signals test to stop
            return -1;
    }

    g_call_count++;
    return 0;  // success
}

void decrypt_data(const unsigned char *input, char *output, int input_len)
{
    (void)input;
    (void)input_len;

    strcpy(output, "press_start_stop");
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
