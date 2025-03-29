#include "can_comms.h"

bool check_is_valid_can_id(canid_t can_id)
{
    bool is_valid = false;

    switch (can_id)
    {
    case CAN_ID_COMMAND:
        is_valid = true;
        break;
    case CAN_ID_SENSOR_READ:
        is_valid = true;
        break;
    default:
        break;
    }

    return is_valid;
}

void parse_input_received(char *input)
{
    printf("%s\n", input);
    fflush(stdout);
    if (strcmp(input, "press_start_stop") == 0)
    {
        start_stop_manual = !start_stop_manual;

        if (start_stop_manual)
        {
            log_toggle_event("Stop/Start: System Activated");
        }
        else
        {
            log_toggle_event("Stop*/Start: System Deactivated");
        }
    }
    /* Check if CAN message is speed */
    if (sscanf(input, "speed: %.1lf", &rec_data.speed) == 1)
    {
        (void)printf("received speed = %.1lf\n", rec_data.speed);
        (void)fflush(stdout);
    }
    /* Check if CAN message is internal temperature */
    if (sscanf(input, "in_temp: %d", &rec_data.internal_temp) == 1)
    {
        (void)printf("received in_temp = %d\n", rec_data.internal_temp);
        (void)fflush(stdout);
    }
    /* Check if CAN message is external temperature */
    if (sscanf(input, "ex_temp: %d", &rec_data.external_temp) == 1)
    {
        (void)printf("received ex_temp = %d\n", rec_data.external_temp);
        (void)fflush(stdout);
    }
    /* Check if CAN message is door open */
    if (sscanf(input, "door: %d", &rec_data.door_open) == 1)
    {
        (void)printf("received door = %d\n", rec_data.door_open);
        (void)fflush(stdout);
    }
    /* Check if CAN message is tilt angle */
    if (sscanf(input, "tilt: %.1lf", &rec_data.tilt_angle) == 1)
    {
        (void)printf("received tilt = %.1lf\n", rec_data.tilt_angle);
        (void)fflush(stdout);
    }
    /* Check if CAN message is acceleration sensor */
    if (sscanf(input, "accel: %d", &rec_data.accel) == 1)
    {
        (void)printf("received accel = %d\n", rec_data.accel);
        (void)fflush(stdout);
    }
    /* Check if CAN message is braking sensor*/
    if (sscanf(input, "brake: %d", &rec_data.brake) == 1)
    {
        (void)printf("received brake = %d\n", rec_data.brake);
        (void)fflush(stdout);
    }
    /* Check if CAN message is temperature setpoint */
    if (sscanf(input, "temp_set: %d", &rec_data.temp_set) == 1)
    {
        (void)printf("received temp_set = %d\n", rec_data.temp_set);
        (void)fflush(stdout);
    }
    /* Check if CAN message is battery SoC */
    if (sscanf(input, "batt_soc: %.1lf", &rec_data.batt_soc) == 1)
    {
        (void)printf("received batt_soc = %.1lf\n", rec_data.batt_soc);
        (void)fflush(stdout);
    }
    /* Check if CAN message is battery voltage */
    if (sscanf(input, "batt_volt: %.1lf", &rec_data.batt_volt) == 1)
    {
        (void)printf("received batt_volt = %.1lf\n", rec_data.batt_volt);
        (void)fflush(stdout);
    }
    /* Check if CAN message is engine temperature */
    if (sscanf(input, "engi_temp: %.1lf", &rec_data.engi_temp) == 1)
    {
        (void)printf("received engi_temp = %.1lf\n", rec_data.engi_temp);
        (void)fflush(stdout);
    }
    /* Check if CAN message is gear */
    if (sscanf(input, "gear: %d", &rec_data.gear) == 1)
    {
        (void)printf("received gear = %d\n", rec_data.gear);
        (void)fflush(stdout);
    }
}

void process_received_frame(int sock)
{
    struct can_frame frame;
    unsigned char encrypted_data[AES_BLOCK_SIZE];
    char decrypted_message[AES_BLOCK_SIZE];
    int received_bytes = 0;
    char message_log[LOG_MESSAGE_SIZE];

    for(;;){
        if (receive_can_frame(sock, &frame) == 0)
        {
            if (check_is_valid_can_id(frame.can_id))
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
                        parse_input_received(decrypted_message);
                        received_bytes = 0;
                    }
                }
                else
                {
                    (void)printf("Warning: Unexpected frame size (%d bytes). Ignoring.\n", frame.can_dlc);
                    (void)fflush(stdout);
                }
            }
        }
    }
}