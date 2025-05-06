#include "dashboard_func.h"
#include <time.h>

#define CAN_DATA_LENGTH (8)
#define PROCESS_TIMEOUT (100000000L)
#define NANO_TO_SEC (1000000000L)

Actuators actuators = {0};

// Shared buffer for CAN messages
static CanBuffer can_buffer;

int sock_dash;

bool test_mode_dash = false;

// Initialize buffer (call once at startup)
void init_can_buffer(void) {
    can_buffer.head = 0;
    can_buffer.tail = 0;
    sem_init(&can_buffer.sem, 0, 0);
    pthread_mutex_init(&can_buffer.mutex, NULL);
    pthread_cond_init(&can_buffer.cond, NULL);
    memset(can_buffer.messages, 0, sizeof(can_buffer.messages));  // Clear all slots
}

// Cleanup (call before exit)
void cleanup_can_buffer(void) {
    sem_destroy(&can_buffer.sem);
    pthread_mutex_destroy(&can_buffer.mutex);
}

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

void parse_input_received(char *input)
{
    process_user_commands(input);
    process_engine_commands(input);
    process_sensor_readings(input);
    process_errors(input);
}

void process_user_commands(char *input)
{
    if (strcmp(input, "press_start_stop") == 0)
    {
        actuators.start_stop_active = !actuators.start_stop_active;

        if (actuators.start_stop_active)
        {
            log_toggle_event("[INFO] System Activated");
            update_value_panel(panel_dash, SYSTEM_ST_ROW, "ON", GREEN_TEXT);
            add_to_log(panel_log, "System Manually Activated");
        }
        else
        {
            log_toggle_event("[INFO] System Deactivated");
            update_value_panel(panel_dash, SYSTEM_ST_ROW, "OFF", RED_TEXT);
            add_to_log(panel_log, "System Manually Deactivated");
        }
    }
}

void process_engine_commands(char *input)
{
    if (actuators.error_system == 1)
    {
        // only error print, error log is defined elsewhere
        update_value_panel(panel_dash, ENGINE_ST_ROW, "ERR", RED_TEXT);
    }
    else if (strcmp(input, "ENGINE OFF") == 0)
    {    
        log_toggle_event("[INFO] Engine Deactivated by Stop/Start");
        update_value_panel(panel_dash, ENGINE_ST_ROW, "OFF", RED_TEXT);
        add_to_log(panel_log, "Engine Deactivated - Stop/Start");

        char sys_deact[4];
        snprintf(sys_deact, sizeof(sys_deact), "%d", ++num_deactivs);
        update_value_panel(panel_dash, NUM_SYS_ACTIV, sys_deact, NORMAL_TEXT);
        add_to_log(panel_log, "Engine Deactivated - Stop/Start");
    }
    else if (strcmp(input, "RESTART") == 0)
    {
        log_toggle_event("[INFO] Engine Activated by Stop/Start");
        update_value_panel(panel_dash, ENGINE_ST_ROW, "ON", GREEN_TEXT);
        add_to_log(panel_log, "Engine Activated - Stop/Start");
    }
}

void process_sensor_readings(char *input)
{
    char result[MAX_VALUE_LENGTH];

    /* Check if CAN message is speed */
    if (sscanf(input, "speed: %lf", &actuators.speed) == 1)
    {
        snprintf(result, sizeof(result), "%.1lf", actuators.speed);
        update_value_panel(panel_dash, SPEED_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is internal temperature */
    else if (sscanf(input, "in_temp: %d", &actuators.internal_temp) == 1)
    {
        snprintf(result, sizeof(result), "%d", actuators.internal_temp);
        update_value_panel(panel_dash, IN_TEMP_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is external temperature */
    else if (sscanf(input, "ex_temp: %d", &actuators.external_temp) == 1)
    {
        snprintf(result, sizeof(result), "%d", actuators.external_temp);
        update_value_panel(panel_dash, EXT_TEMP_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is door open */
    else if (sscanf(input, "door: %d", &actuators.door_status) == 1)
    {
        //snprintf(result, sizeof(result), "%d", actuators.door_status);
        const char* door_status = actuators.door_status ? "Yes" : "No";
        update_value_panel(panel_dash, DOOR_ROW, door_status, NORMAL_TEXT);
    }
    /* Check if CAN message is battery SoC */
    else if (sscanf(input, "batt_soc: %lf", &actuators.batt_soc) == 1)
    {
        snprintf(result, sizeof(result), "%.1lf", actuators.batt_soc);
        update_value_panel(panel_dash, BATT_SOC_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is battery voltage */
    else if (sscanf(input, "batt_volt: %lf", &actuators.batt_volt) == 1)
    {
        snprintf(result, sizeof(result), "%.1lf", actuators.batt_volt);
        update_value_panel(panel_dash, BATT_VOLT_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is engine temperature */
    else if (sscanf(input, "engi_temp: %lf", &actuators.engi_temp) == 1)
    {
        snprintf(result, sizeof(result), "%.1lf", actuators.engi_temp);
        update_value_panel(panel_dash, ENGI_TEMP_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is gear */
    else if (sscanf(input, "gear: %d", &actuators.gear) == 1)
    {
        /* if (actuators.speed > 0.0F)
        {
            strncpy(result, "D", sizeof(result));
        }
        else
        {
            strncpy(result, "P", sizeof(result));
        } */
        snprintf(result, sizeof(result), "%s", actuators.gear ? "D" : "P");
        update_value_panel(panel_dash, GEAR_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is acceleration sensor */
    else if (sscanf(input, "accel: %d", &actuators.accel) == 1)
    {
        snprintf(result, sizeof(result), "%d", actuators.accel);
        update_value_panel(panel_dash, ACCEL_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is braking sensor*/
    else if (sscanf(input, "brake: %d", &actuators.brake) == 1)
    {
        snprintf(result, sizeof(result), "%d", actuators.brake);
        update_value_panel(panel_dash, BRAKE_ROW, result, NORMAL_TEXT);
    }
    /* Check if CAN message is tilt angle */
    else if (sscanf(input, "tilt: %lf", &actuators.tilt_angle) == 1)
    {
        snprintf(result, sizeof(result), "%.1lf", actuators.tilt_angle);
        update_value_panel(panel_dash, TILT_ROW, result, NORMAL_TEXT);
    }
}

void process_errors(char *input)
{
    if (strcmp(input, "error_battery_drop") == 0)
    {
        log_toggle_event("[INFO] Engine Restart Failed Due to Battery Tension Drop");
        add_to_log(panel_log, "Engine Restart Failed - Battery Drop");
    }
    else if (strcmp(input, "error_battery") == 0)
    {
        log_toggle_event("[INFO] Engine Restart Failed Due to Low Battery SoC or Tension Under the Threshold");
        add_to_log(panel_log, "Engine Restart Failed - Battery");
    }
    else if (strcmp(input, "error_disabled") == 0)
    {
        actuators.start_stop_active = false;
        actuators.error_system = 1;
        log_toggle_event("[INFO] System Disabled Due to an Error");
        add_to_log(panel_log, "System Disabled - Error");
    }
}

void* process_frame_thread(void* arg) {
    (void)arg;
    struct timespec timeout;

    while(!test_mode_dash) {
        pthread_mutex_lock(&can_buffer.mutex);
        
        // Calculate absolute timeout
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += PROCESS_TIMEOUT;

        // Handle overflow
        if (timeout.tv_nsec >= NANO_TO_SEC)
        {
            timeout.tv_sec += timeout.tv_nsec / NANO_TO_SEC;
            timeout.tv_nsec %= NANO_TO_SEC;
        }

        // Handle timeout
        while (can_buffer.tail == can_buffer.head) {
            if (pthread_cond_timedwait(&can_buffer.cond, &can_buffer.mutex, &timeout) != 0) {
                // Timeout occurred
                break;
            }
        }

        // Process all available messages
        while (can_buffer.tail != can_buffer.head) {
            // Update panel_dash with the decoded data
            parse_input_received(can_buffer.messages[can_buffer.tail].decrypted);

            // Clear the processed message slot
            memset(&can_buffer.messages[can_buffer.tail], 0, sizeof(CanMessage));
            can_buffer.tail = (can_buffer.tail + 1) % MAX_PENDING_FRAMES;
        }

        pthread_mutex_unlock(&can_buffer.mutex);
    }
    return NULL;
}

void* can_receiver_thread(void* arg) {
    (void)arg;
    struct can_frame frame;
    unsigned char encrypted_data[AES_BLOCK_SIZE];
    int received_bytes = 0;
    
    #ifdef UNIT_TEST
    while (!test_mode_dash)
#else
    for (;;)
#endif
    {
        if (receive_can_frame(sock_dash, &frame) == 0) {
            if (check_is_valid_can_id(frame.can_id) && 
                (frame.can_dlc == CAN_DATA_LENGTH)) 
            {
                // Accumulate data until we have a full block
                memcpy(encrypted_data + received_bytes, frame.data, frame.can_dlc);
                received_bytes += frame.can_dlc;

                if (received_bytes >= AES_BLOCK_SIZE) {
                    pthread_mutex_lock(&can_buffer.mutex);
                    // Overwrite oldest message if buffer is full
                    if ((can_buffer.head + 1) % MAX_PENDING_FRAMES == can_buffer.tail) {
                        can_buffer.tail = (can_buffer.tail + 1) % MAX_PENDING_FRAMES;
                        add_to_log(panel_log, "WARN: Buffer full - dropped oldest frame");
                    }

                    // Store the new message
                    can_buffer.messages[can_buffer.head].frame = frame;
                    decrypt_data(encrypted_data, 
                               can_buffer.messages[can_buffer.head].decrypted, 
                               AES_BLOCK_SIZE);

                    // Update head and notify main thread
                    can_buffer.head = (can_buffer.head + 1) % MAX_PENDING_FRAMES;
                    sem_post(&can_buffer.sem);

                    // Log raw frame (to panel_log)
                    char log_msg[MAX_MSG_WIDTH];
                    int offset = snprintf(log_msg, sizeof(log_msg), "RCV: ");
                    for (int i = 0; i < frame.can_dlc; i++) {
                        offset += snprintf(log_msg + offset, sizeof(log_msg) - offset,
                                 " %02X", frame.data[i]);
                    }
                    add_to_log(panel_log, log_msg);

                    pthread_mutex_unlock(&can_buffer.mutex);
                    received_bytes = 0;  // Reset for next frame
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
    return NULL;
}

