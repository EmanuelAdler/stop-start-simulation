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
            is_valid = true;
            break;        
        default:
            break;
    }
    
    return is_valid;
}

void print_dashboard_status() 
{
    (void)printf("\n=== Dashboard Status ===\n");
    (void)printf("Stop/Start button: %d\n", actuators.start_stop_active);
    
    // get bcm data
    VehicleData *vehicle = &vehicle_data[simu_curr_step];
    
    (void)printf("Battery SOC: %.2f%%\n", vehicle->batt_soc);
    (void)printf("Battery Voltage: %.2fV\n", vehicle->batt_volt);
    (void)printf("Door Open: %s\n", vehicle->door_open ? "Yes" : "No");

    (void)printf("=============================\n");
    (void)fflush(stdout);
}

void parse_input_received(char* input)
{
    if (strcmp(input, "press_start_stop") == 0)
    {
        actuators.start_stop_active = !actuators.start_stop_active;

        if (actuators.start_stop_active) 
        {
            log_toggle_event("[INFO] System Activated");
        } 
        else 
        {
            log_toggle_event("[INFO] System Deactivated");
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
        log_toggle_event("[INFO] Engine Desactivated by star/stop");
    } 
    else if (strcmp(input, "RESTART") == 0)
    {  
        log_toggle_event("[INFO] Engine Activated by star/stop");
    }
}

void process_errors(char* input)
{
    if (strcmp(input, "error_battery_drop") == 0)
    {
        log_toggle_event("[INFO] Engine Restart Failed due to battery tension drop");
    } 
    else if (strcmp(input, "error_battery_low") == 0)
    {  
        log_toggle_event("[INFO] Engine Restart Failed due to battery SOC or tension is under the thresrold");
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
        #ifdef UNIT_TEST
        else
        {
            break;
        }
        #endif
    }
}
