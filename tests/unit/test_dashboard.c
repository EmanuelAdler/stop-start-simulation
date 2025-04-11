#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/dashboard/dashboard_func.h"
#include "../../src/common_includes/logging.h"

#define MOCK_SOCKET       (999)
#define FILE_LINE_SIZE    (256)
#define CAN_ID_MOCK       (0x7A0U)

static const double kDelta             = 0.001;
static const double kBattSocReceived   = 55.0;
static const double kBattVoltReceived  = 12.7;

//-------------------------------------
// Setup the CUnit Suite
//-------------------------------------
static int init_suite(void)  { return 0; }
static int clean_suite(void) { return 0; }

typedef struct
{
    const char *filepath;
    const char *substring;
} f_susbtring_data;

/* Utility to search for a substring in a file */
static bool file_contains_substring(f_susbtring_data struct_f_subs)
{
    FILE *fpath = fopen(struct_f_subs.filepath, "r");
    if (!fpath)
    {
        return false;
    }

    char line[FILE_LINE_SIZE];
    bool found = false;
    while (fgets(line, sizeof(line), fpath)) {
        if (strstr(line, struct_f_subs.substring)) {
            found = true;
            break;
        }
    }
    fclose(fpath);
    return found;
}

//-------------------------------------
// Test 1: Test process received frames
//-------------------------------------
/* 
 * This test calls process_received_frame() with our stubs:
 *  - 2 valid frames => triggers decrypt+parse_input_received
 *  - 1 invalid-size frame => triggers warning
 *  - Then returns -1 => we break out of the loop 
 */
void test_process_received_frame(void)
{
    // 1) Setup logging so "press_start_stop" toggles system & logs
    set_log_file_path("/tmp/test_dashboard_frame.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // 2) We can ensure the actuators are initially off
    actuators.start_stop_active = false;

    // 3) Call the function once. It will loop until stub returns -1.
    process_received_frame(MOCK_SOCKET);

    // 4) Cleanup
    cleanup_logging_system();

    // 5) Check the log for "[INFO] System Activated" 
    //    if parse_input_received toggled the system on.
    f_susbtring_data params_str;

    params_str.filepath = "/tmp/test_dashboard_frame.log";
    params_str.substring = "[INFO] System Activated";

    CU_ASSERT_TRUE(file_contains_substring(params_str));
}

//-------------------------------------
// Test 2: Test the output for print dashboard status
//-------------------------------------
void test_print_dashboard_status(void)
{
    // 1) Set the actuator state as we like
    actuators.start_stop_active = false;
    actuators.error_system = true;

    // 2) Saves the state of the “real” stdout
    int saved_stdout_fd = dup(STDOUT_FILENO);

    // 3) Open temporary file
    const char *stdout_file = "/tmp/test_dashboard_stdout.txt";
    FILE *fpath = fopen(stdout_file, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fpath);

    // 4) Redirect stdout to this file
    int temp_fd = fileno(fpath);
    dup2(temp_fd, STDOUT_FILENO);

    // 5) Call the function
    print_dashboard_status();

    // 6) Restore original stdout
    fflush(stdout);
    dup2(saved_stdout_fd, STDOUT_FILENO);
    close(saved_stdout_fd);
    fclose(fpath);

    // 7) Now check the file for expected text
    f_susbtring_data params_str;

    params_str.filepath = stdout_file;
    params_str.substring = "=== Dashboard Status ===";

    CU_ASSERT_TRUE(file_contains_substring(params_str));

    params_str.substring = "Stop/Start button: 0";
    CU_ASSERT_TRUE(file_contains_substring(params_str));

    params_str.substring = "Battery SOC: 0.0";
    CU_ASSERT_TRUE(file_contains_substring(params_str));

    params_str.substring = "Battery Voltage: 0.0";
    CU_ASSERT_TRUE(file_contains_substring(params_str));

    params_str.substring = "Door Open: No";
    CU_ASSERT_TRUE(file_contains_substring(params_str));

    params_str.substring = "System Disabled Warning: Yes";
    CU_ASSERT_TRUE(file_contains_substring(params_str));
}

//-------------------------------------
// Test 3: Test the variants of inputs received by the dashboard
//-------------------------------------
void test_parse_input_variants(void)
{
    // 1) Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_dashboard_variants.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // 2) Redirect stdout to a temporary file
    const char *stdout_file = "/tmp/test_dashboard_variants_stdout.txt";
    FILE *fp_stdout = fopen(stdout_file, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp_stdout);

    int saved_stdout_fd = dup(STDOUT_FILENO);
    int temp_fd = fileno(fp_stdout);
    dup2(temp_fd, STDOUT_FILENO);

    // 3) Test case 1: "press_start_stop" => activates the system
    parse_input_received("press_start_stop");
    // Assert that start_stop_active became true
    CU_ASSERT_TRUE(actuators.start_stop_active);
    // Assert log message "[INFO] System Activated" exists
    f_susbtring_data param_activated = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] System Activated"
    };
    CU_ASSERT_TRUE(file_contains_substring(param_activated));
    // Assert that the message is in the stdout file
    f_susbtring_data activated_status_title = { stdout_file, "[INFO] System Activated" };
    CU_ASSERT_TRUE(file_contains_substring(activated_status_title));

    // 4) Test case 2: "press_start_stop" => deactivates the system
    parse_input_received("press_start_stop");
    CU_ASSERT_FALSE(actuators.start_stop_active);
    f_susbtring_data param_deactivated = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] System Deactivated"
    };
    CU_ASSERT_TRUE(file_contains_substring(param_deactivated));
    // Assert that the message is in the stdout file
    f_susbtring_data deactivated_status_title = { stdout_file, "[INFO] System Deactivated" };
    CU_ASSERT_TRUE(file_contains_substring(deactivated_status_title));

    // 5) Test case 3: "show_dashboard" => calls print_dashboard_status()
    parse_input_received("show_dashboard");

    // Restore stdout to stop redirection
    fflush(stdout);
    dup2(saved_stdout_fd, STDOUT_FILENO);
    close(saved_stdout_fd);
    fclose(fp_stdout);

    // Assert print_dashboard_status wrote to the stdout file
    f_susbtring_data dash_status_title = { stdout_file, "=== Dashboard Status ===" };
    CU_ASSERT_TRUE(file_contains_substring(dash_status_title));
    f_susbtring_data dash_stop_start = { stdout_file, "Stop/Start button: 0" };
    CU_ASSERT_TRUE(file_contains_substring(dash_stop_start));

    // 6) Test case 4: "ENGINE OFF" => logs "[INFO] Engine Deactivated by Stop/Start"
    parse_input_received("ENGINE OFF");
    f_susbtring_data engine_off_str = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Deactivated by Stop/Start"
    };
    CU_ASSERT_TRUE(file_contains_substring(engine_off_str));

    // 7) Test case 5: "RESTART" => logs "[INFO] Engine Activated by Stop/Start"
    parse_input_received("RESTART");
    f_susbtring_data engine_restart_str = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Activated by Stop/Start"
    };
    CU_ASSERT_TRUE(file_contains_substring(engine_restart_str));

    // 8) Test case 6: Sensor readings (batt_soc, batt_volt, door)
    parse_input_received("batt_soc: 55.0");
    CU_ASSERT_DOUBLE_EQUAL(actuators.batt_soc, kBattSocReceived, kDelta);

    parse_input_received("batt_volt: 12.7");
    CU_ASSERT_DOUBLE_EQUAL(actuators.batt_volt, kBattVoltReceived, kDelta);

    parse_input_received("door: 1");
    CU_ASSERT_TRUE(actuators.door_status);

    // 9) Test case 7: Error messages
    parse_input_received("error_battery_drop");
    f_susbtring_data err_drop = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Restart Failed Due to Battery Tension Drop"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_drop));

    parse_input_received("error_battery_low");
    f_susbtring_data err_low = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Restart Failed Due to Battery SoC or Tension Under the Threshold"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_low));

    parse_input_received("system_disabled_error");
    f_susbtring_data err_disab = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] System Disabled Due to an Error"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_disab));
    CU_ASSERT_FALSE(actuators.start_stop_active);
    CU_ASSERT_TRUE(actuators.error_system);

    // 10) Finalize and clean up
    cleanup_logging_system();
}

//-------------------------------------
// Test 4: Test invalid CAN ID for dashboard
//-------------------------------------
void test_invalid_can_id_dashboard(void)
{
    CU_ASSERT_FALSE(check_is_valid_can_id(CAN_ID_MOCK));
}

int main(void)
{
    // Initialize CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("DashboardSuite", init_suite, clean_suite);
    if (!suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests
    CU_add_test(suite, "process_received_frame coverage", test_process_received_frame);
    CU_add_test(suite, "print_dashboard_status", test_print_dashboard_status);
    CU_add_test(suite, "parse_input_variants", test_parse_input_variants);
    CU_add_test(suite, "invalid_can_id_dashboard", test_invalid_can_id_dashboard);

    // Run all tests in verbose mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
