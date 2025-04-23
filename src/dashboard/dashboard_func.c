#include "../common_includes/can_id_list.h"
#include "../common_includes/can_socket.h"
#include "../common_includes/logging.h"
#include "dashboard_func.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAN_DATA_LENGTH    (8)

Actuators actuators = {0};

bool check_is_valid_can_id(canid_t can_id)
{
    bool is_valid = false;
    
    switch (can_id)
    {
        case CAN_ID_COMMAND:
        case CAN_ID_ERROR_DASH:
        case CAN_ID_ECU_RESTART:
        case CAN_ID_SENSOR_READ:
            is_valid = true;
            break;        
        default:
            break;
    }
    
    return is_valid;
}

/**
 * @brief Process the output for print dashboard status.
 * @requirement SWR5.2
 * @requirement SWR6.3
 */
void print_dashboard_status() 
{
    (void)printf("\n=== Dashboard Status ===\n");
    (void)printf("Stop/Start button: %d\n", actuators.start_stop_active);       
    (void)printf("Battery SOC: %.2f%%\n", actuators.batt_soc);
    (void)printf("Battery Voltage: %.2fV\n", actuators.batt_volt);
    (void)printf("Door Open: %s\n", actuators.door_status ? "Yes" : "No");
    (void)printf("System Disabled Warning: %s\n", actuators.error_system ? "Yes" : "No");

    (void)printf("=============================\n");
    (void)fflush(stdout);
}

/**
 * @brief Process the variants of inputs received by the dashboard.
 * @requirement SWR1.3
 * @requirement SWR4.5
 * @requirement SWR5.1
 * @requirement SWR5.3
 */
void parse_input_received(char* input)
{
    process_user_commands(input);
    process_engine_commands(input);
    process_sensor_readings(input);
    process_errors(input);
}

void process_user_commands(char* input)
{
    if (strcmp(input, "press_start_stop") == 0)
    {
        actuators.start_stop_active = !actuators.start_stop_active;

        if (actuators.start_stop_active) 
        {
            log_toggle_event("[INFO] System Activated");
            (void)printf("[INFO] System Activated\n");
            (void)fflush(stdout);
        } 
        else 
        {
            log_toggle_event("[INFO] System Deactivated");
            (void)printf("[INFO] System Deactivated\n");
            (void)fflush(stdout);
        }
    }
    else if (strcmp(input, "show_dashboard") == 0)
    {
        print_dashboard_status();
    }
}

void process_engine_commands(char* input)
{
    if (strcmp(input, "ENGINE OFF") == 0)
    {
        log_toggle_event("[INFO] Engine Deactivated by Stop/Start");
    } 
    else if (strcmp(input, "RESTART") == 0)
    {  
        log_toggle_event("[INFO] Engine Activated by Stop/Start");
    }
}

void process_sensor_readings(char* input)
{
    /* Check if CAN message is battery SoC */
    if (sscanf(input, "batt_soc: %lf", &actuators.batt_soc) == 1)
    {
        (void)printf("received batt_soc = %.1lf\n", actuators.batt_soc);
        (void)fflush(stdout);
    }
    /* Check if CAN message is battery voltage */
    else if (sscanf(input, "batt_volt: %lf", &actuators.batt_volt) == 1)
    {
        (void)printf("received batt_volt = %lf\n", actuators.batt_volt);
        (void)fflush(stdout);
    }
    /* Check if CAN message is door open */
    else if (sscanf(input, "door: %d", &actuators.door_status) == 1)
    {
        (void)printf("received door = %d\n", actuators.door_status);
        (void)fflush(stdout);
    }
}

void process_errors(char* input)
{
    if (strcmp(input, "error_battery_drop") == 0)
    {
        log_toggle_event("[INFO] Engine Restart Failed Due to Battery Tension Drop");
    } 
    else if (strcmp(input, "error_battery_low") == 0)
    {  
        log_toggle_event("[INFO] Engine Restart Failed Due to Battery SoC or Tension Under the Threshold");
    }
    else if (strcmp(input, "system_disabled_error") == 0)
    {  
        actuators.start_stop_active = false;
        actuators.error_system = 1;
        log_toggle_event("[INFO] System Disabled Due to an Error");
    }
}

void process_received_frame(int sock) 
{
    struct can_frame frame;
    unsigned char encrypted_data[AES_BLOCK_SIZE];
    char decrypted_message[AES_BLOCK_SIZE];
    int received_bytes = 0;

    #ifdef UNIT_TEST
    while (true)
    #else
    for (;;)
    #endif
    {
        if (receive_can_frame(sock, &frame) == 0) 
        {
            if (check_is_valid_can_id(frame.can_id))
            {
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
        #ifdef UNIT_TEST
        else
        {
            break;
        }
        #endif
    }
}
