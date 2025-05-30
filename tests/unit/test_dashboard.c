#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/common_includes/can_id_list.h"
#include "../../src/common_includes/can_socket.h"
#include "../../src/dashboard/dashboard_func.h"
#include "../../src/common_includes/logging.h"

#include "mock_ncurses.h"

#define MOCK_SOCKET (999)
#define FILE_LINE_SIZE (256)
#define CAN_ID_MOCK (0x7A0U)
#define SLEEP_TIME_US_TEST (200000)

static const double kDelta = 0.001;
static const double kSpeedReceived = 48.0;
static const int kintempReceived = 22;
static const int kextempReceived = 27;
static const double kBattSocReceived = 55.0;
static const double kBattVoltReceived = 12.7;
static const double kengiReceived = 85.0;
static const double ktiltReceived = 7.0;

//-------------------------------------
// Setup the CUnit Suite
//-------------------------------------
static int init_suite(void) { return 0; }
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
    while (fgets(line, sizeof(line), fpath))
    {
        if (strstr(line, struct_f_subs.substring))
        {
            found = true;
            break;
        }
    }
    fclose(fpath);
    return found;
}

//-------------------------------------
// Test 1: Test process received frames
/* 
 * This test calls process_received_frame() with our stubs:
 *  - 2 valid frames => triggers decrypt+parse_input_received
 *  - 1 invalid-size frame => triggers warning
 *  - Then returns -1 => we break out of the loop
 */
/**
 * @test test_process_received_frame
 * @brief Test process received frames
 * @req SWR1.2
 * @req SWR1.4
 * @file unit/test_dashboard.c
 */
void test_process_received_frame(void)
{
    // 1) Setup logging so "press_start_stop" toggles system & logs
    set_log_file_path("/tmp/test_dashboard_frame.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // 2) We can ensure the actuators are initially off
    actuators.start_stop_active = false;
    test_mode_dash = false;

    // 3) Call the function once. It will loop until stub returns -1.
    pthread_t thd;
    pthread_t thd2;

    sock_dash = MOCK_SOCKET;

    init_can_buffer();

    pthread_create(&thd, NULL, can_receiver_thread, NULL);
    pthread_create(&thd2, NULL, process_frame_thread, NULL);

    sleep(1);

    test_mode_dash = true;

    // 4) Cleanup
    cleanup_can_buffer();
    cleanup_logging_system();

    // 5) Check the log for "[INFO] System Activated"
    //    if parse_input_received toggled the system on.
    f_susbtring_data params_str;

    params_str.filepath = "/tmp/test_dashboard_frame.log";
    params_str.substring = "[INFO] System Activated";

    CU_ASSERT_TRUE(file_contains_substring(params_str));
}

//-------------------------------------
// Test 2: Test the variants of inputs received by the dashboard
/**
 * @test test_parse_input_variants
 * @brief Test the variants of inputs received by the dashboard
 * @req SWR1.3
 * @req SWR4.5
 * @req SWR5.1
 * @req SWR5.2
 * @req SWR5.3
 * @req SWR6.3
 * @file unit/test_dashboard.c
 */
void test_parse_input_variants(void)
{
    actuators.start_stop_active = false;

    // 1) Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_dashboard_variants.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // 2) Test case 1: "press_start_stop" => activates the system
    parse_input_received("press_start_stop");
    // Assert that start_stop_active became true
    CU_ASSERT_TRUE(actuators.start_stop_active);
    // Assert log message "[INFO] System Activated" exists
    f_susbtring_data param_activated = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] System Activated"};
    CU_ASSERT_TRUE(file_contains_substring(param_activated));

    // 3) Test case 2: "press_start_stop" => deactivates the system
    parse_input_received("press_start_stop");
    CU_ASSERT_FALSE(actuators.start_stop_active);
    f_susbtring_data param_deactivated = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] System Deactivated"};
    CU_ASSERT_TRUE(file_contains_substring(param_deactivated));

    // 4) Test case 3: "ENGINE OFF" => logs "[INFO] Engine Deactivated by Stop/Start"
    parse_input_received("ENGINE OFF");
    f_susbtring_data engine_off_str = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Deactivated by Stop/Start"};
    CU_ASSERT_TRUE(file_contains_substring(engine_off_str));

    // 5) Test case 4: "RESTART" => logs "[INFO] Engine Activated by Stop/Start"
    parse_input_received("RESTART");
    f_susbtring_data engine_restart_str = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Activated by Stop/Start"};
    CU_ASSERT_TRUE(file_contains_substring(engine_restart_str));

    // 6) Test case 5: Sensor readings (batt_soc, batt_volt, door)
    parse_input_received("speed: 48.0");
    CU_ASSERT_DOUBLE_EQUAL(actuators.speed, kSpeedReceived, kDelta);

    parse_input_received("in_temp: 22");
    CU_ASSERT_EQUAL(actuators.internal_temp, kintempReceived);

    parse_input_received("ex_temp: 27");
    CU_ASSERT_EQUAL(actuators.external_temp, kextempReceived);

    parse_input_received("batt_soc: 55.0");
    CU_ASSERT_DOUBLE_EQUAL(actuators.batt_soc, kBattSocReceived, kDelta);

    parse_input_received("batt_volt: 12.7");
    CU_ASSERT_DOUBLE_EQUAL(actuators.batt_volt, kBattVoltReceived, kDelta);

    parse_input_received("door: 1");
    CU_ASSERT_TRUE(actuators.door_status);

    parse_input_received("engi_temp: 85.0");
    CU_ASSERT_DOUBLE_EQUAL(actuators.engi_temp, kengiReceived, kDelta);

    // gear and (speed = 0) or (speed > 0)

    parse_input_received("gear: 0");
    CU_ASSERT_FALSE(actuators.gear);

    parse_input_received("speed: 0.0");
    parse_input_received("gear: 1");
    CU_ASSERT_TRUE(actuators.gear);

    parse_input_received("accel: 1");
    CU_ASSERT_TRUE(actuators.accel);

    parse_input_received("brake: 1");
    CU_ASSERT_TRUE(actuators.brake);

    parse_input_received("tilt: 7.0");
    CU_ASSERT_DOUBLE_EQUAL(actuators.tilt_angle, ktiltReceived, kDelta);

    // 7) Test case 6: Error messages
    parse_input_received("error_battery_drop");
    f_susbtring_data err_drop = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Restart Failed Due to Battery Tension Drop"};
    CU_ASSERT_TRUE(file_contains_substring(err_drop));

    parse_input_received("error_battery");
    f_susbtring_data err_low = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] Engine Restart Failed Due to Low Battery SoC or Tension Under the Threshold"};
    CU_ASSERT_TRUE(file_contains_substring(err_low));

    parse_input_received("error_disabled");
    f_susbtring_data err_disab = {
        "/tmp/test_dashboard_variants.log",
        "[INFO] System Disabled Due to an Error"};
    CU_ASSERT_TRUE(file_contains_substring(err_disab));
    CU_ASSERT_FALSE(actuators.start_stop_active);
    CU_ASSERT_TRUE(actuators.error_system);
}

#define TEST_VALUE_PANEL_HEIGHT 20
#define TEST_VALUE_PANEL_WIDTH 40

#define TEST_LOG_PANEL_HEIGHT 20
#define TEST_LOG_PANEL_WIDTH 50
#define TEST_LOG_PANEL_OFFSET 45
#define TEST_MSG_MAX_SIZE   44

//-------------------------------------
// Test 3: Test the panel functions
//-------------------------------------
void test_panels(void)
{
    // 1) Create a value panel

    panel_dash = create_value_panel(
        (Size){TEST_VALUE_PANEL_HEIGHT, TEST_VALUE_PANEL_WIDTH}, 
        (Position){1, 1}, 
        "Test_dash"
    );
    CU_ASSERT_PTR_NOT_NULL(panel_dash);

    // 2) Update the value panel

    update_value_panel(panel_dash, 1, "35", NORMAL_TEXT);
    char *read_value_result = read_value_panel();
    CU_ASSERT_STRING_EQUAL(read_value_result, "35");

    // 3) Create a log panel

    panel_log = create_log_panel(
        (Size){TEST_LOG_PANEL_HEIGHT, TEST_LOG_PANEL_WIDTH}, 
        (Position){1, TEST_LOG_PANEL_OFFSET}, 
        "Test_log"
    );
    CU_ASSERT_PTR_NOT_NULL(panel_log);

    // 4) Update log panel

    add_to_log(panel_log, "Receive: CAN message");

    // Simulate timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[TMSTMP_SIZE];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);

    char test_log[TEST_MSG_MAX_SIZE];
    // Store the formatted message
    snprintf(test_log, sizeof(test_log),
             "[%s] %s", timestamp, "Receive: CAN message");

    char *read_log_result = read_log_panel();
    CU_ASSERT_STRING_EQUAL(read_log_result, test_log);
}

//-------------------------------------
// Test 4: Test invalid CAN ID for dashboard
//-------------------------------------
void test_invalid_can_id_dashboard(void)
{
    // Test with an invalid CAN ID (should make check_is_valid_can_id return false)
    #define INVALID_CAN_ID      0x6A3

    // Verify the ID is indeed invalid
    CU_ASSERT_FALSE(check_is_valid_can_id(INVALID_CAN_ID));
}

int main(void)
{
    // Initialize CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("DashboardSuite", init_suite, clean_suite);
    if (!suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests
    CU_add_test(suite, "process_received_frame_coverage", test_process_received_frame);
    CU_add_test(suite, "parse_input_variants", test_parse_input_variants);
    CU_add_test(suite, "panels", test_panels);
    CU_add_test(suite, "invalid_can_id_dashboard", test_invalid_can_id_dashboard);

    // Run all tests in verbose mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
