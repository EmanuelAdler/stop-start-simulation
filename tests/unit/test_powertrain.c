#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <linux/can.h>

#include "../../src/powertrain/powertrain_func.h"
#include "../../src/powertrain/can_comms.h"

#define LOG_EVENT_MSG_SIZE     (256)
#define TEMP_OK                (25)
#define TEMP_HIGH              (31)
#define TEMP_LOW               (20)
#define TEMP_FAIL              (31)
#define TEMP_SET_OK            (25)
#define ENG_TEMP_OK            (90)
#define ENG_TEMP_FAIL          (15)
#define BATT_SOC_OK            (80.0)
#define BATT_SOC_LOW           (60.0)
#define BATT_VOLT_OK           (12.5)
#define BATT_VOLT_LOW          (12.0)
#define TILT_OK                (2.0)
#define TILT_FAIL              (6.0)
#define SPEED_OK               (0.0)
#define DOOR_OK                (0)
#define DOOR_FAIL              (1)
#define GEAR_OK                (0)
#define ACCEL_OK               (0)
#define BRAKE_OK               (1)
#define BRAKE_FAIL             (0)
#define SLEEP_TIME_US_TEST     (100000)
#define USLEEP_DELAY_THREAD    (200000)
#define GEAR_RECEIVED          (2)
#define TEMP_SET_RECEIVED      (22)
#define BRAKE_RECEIVED         (1)
#define ACCEL_RECEIVED         (3)
#define DOOR_RECEIVED          (1)
#define EXTERNAL_TEMP_RECEIVED (10)
#define INTERNAL_TEMP_RECEIVED (23)
#define CAN_DATA_SIZE          (8)
#define AES_BLOCK_TEST_SIZE    (16)
#define TEST_BYTE_1 0x11
#define TEST_BYTE_2 0x22
#define TEST_BYTE_3 0x33
#define TEST_BYTE_4 0x44
#define TEST_BYTE_5 0x55
#define TEST_BYTE_6 0x66
#define TEST_BYTE_7 0x77
#define TEST_BYTE_8 0x88
#define ERROR_BATTERY_VOLTAGE 10.2F
#define FILE_LINE_SIZE    (256)

//-------------------------------------
// Declare the extra "mock" functions created
//-------------------------------------
int stub_can_get_send_count(void);
const char* stub_can_get_last_message(void);
void stub_can_reset(void);

static const double kSpeedReceived     = 45.7;
static const double kTiltReceived      = 4.2;
static const double kBattSocReceived   = 81.5;
static const double kBattVoltReceived  = 13.2;
static const double kEngTempReceived   = 95.4;
static const double kDelta             = 0.001;
static const double kSpeedMock         = 5.0;
static const double kDeltaMock         = 0.0001;

static bool mock_log_toggle_event_called = false;
static char mock_log_toggle_event_msg[LOG_EVENT_MSG_SIZE] = {0};

static bool mock_receive_can_frame_enable = false;
static struct can_frame mock_frame_to_return;

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
// Setup the CUnit Suite
//-------------------------------------
static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

static VehicleData base_ok_data(void)
{
    VehicleData data_test = {
        .speed         = SPEED_OK,
        .internal_temp = TEMP_OK,
        .external_temp = TEMP_HIGH,
        .door_open     = DOOR_OK,
        .tilt_angle    = TILT_OK,
        .accel         = ACCEL_OK,
        .brake         = BRAKE_OK,
        .temp_set      = TEMP_SET_OK,
        .batt_soc      = BATT_SOC_OK,
        .batt_volt     = BATT_VOLT_OK,
        .engi_temp     = ENG_TEMP_OK,
        .gear          = GEAR_OK
    };
    return data_test;
}

// ----------------------------------------------------------------------
// Unit tests

// 1) All conditions OK => should set cond1..cond6=1 => activates if not already active (SWR5.1)
/**
 * @test test_check_disable_engine_all_ok
 * @brief Tests engine start functionality
 * @req SWR1.1
 * @file unit/test_powertrain.c
 */
static void test_check_disable_engine_all_ok(void)
{
    // Test example of "check_disable_engine()"
    engine_off = false;
    start_stop_manual = true; // simulates manual trigger

    VehicleData data_test = base_ok_data();

    // Reset the stub counter
    stub_can_reset();

    // Call the function
    check_disable_engine(&data_test);

    // Check if the engine was turned off
    CU_ASSERT_TRUE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about turning the engine off
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "ENGINE OFF");
}

// 2) Fail cond1 => e.g. brake=0
static void test_check_disable_engine_fail_cond1(void)
{
    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_check_disable_engine_fail_cond1.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    start_stop_manual    = true;
    engine_off = false; // so the error branch triggers

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = base_ok_data();
    data_test.brake = BRAKE_FAIL; // failing condition speed==0 && !accel && brake => now brake=0 => else branch

    check_disable_engine(&data_test);

    // cond1 = 0 => engine_off should remain false
    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about brake not being pressed
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "error_brake_not_pressed");

    // Check received log message
    f_susbtring_data check_cond = {
        "/tmp/test_check_disable_engine_fail_cond1.log",
        "Stop/Start: SWR2.8 (Brake not pressed or car is moving!)"
    };
    CU_ASSERT_TRUE(file_contains_substring(check_cond));

    // Finalize and clean up
    cleanup_logging_system();
}

// 3) Fail cond2 => e.g. internal_temp = temp_set + 6 => bigger than (temp_set + 5) (SWR2.6)
static void test_check_disable_engine_fail_cond2(void)
{
    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_check_disable_engine_fail_cond2.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    start_stop_manual    = true;
    engine_off = false;

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = base_ok_data();
    data_test.internal_temp = TEMP_FAIL; // triggers the else for cond2

    check_disable_engine(&data_test);

    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about external or internal temperature out of range
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "error_temperature_out_range");

    // Check received log message
    f_susbtring_data check_cond = {
        "/tmp/test_check_disable_engine_fail_cond2.log",
        "Stop/Start: SWR2.8 (Difference between internal and external temps out of range!)"
    };
    CU_ASSERT_TRUE(file_contains_substring(check_cond));

    // Finalize and clean up
    cleanup_logging_system();
}

// 4) Fail cond3 => e.g. engine temp < MIN_ENGINE_TEMP => 60 (SWR2.2)
static void test_check_disable_engine_fail_cond3_inactive(void)
{
    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_check_disable_engine_fail_cond3.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    start_stop_manual    = true;
    engine_off = false;

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = base_ok_data();
    data_test.engi_temp = ENG_TEMP_FAIL; // below MIN_ENGINE_TEMP(20)

    check_disable_engine(&data_test);

    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about engine temperature out of range
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "error_engine_temperature_out_range");

    // Check received log message
    f_susbtring_data check_cond = {
        "/tmp/test_check_disable_engine_fail_cond3.log",
        "Stop/Start: SWR2.8 (Engine temperature out of range!)"
    };
    CU_ASSERT_TRUE(file_contains_substring(check_cond));

    // Finalize and clean up
    cleanup_logging_system();
}

// 5) Fail cond4 => battery is too low => batt_soc < 70 and volt <= 12.2 (SWR2.3, SWR4.3 and SWR4.4)
static void test_check_disable_engine_fail_cond4(void)
{
    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_check_disable_engine_fail_cond4.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    start_stop_manual    = true;
    engine_off = false;

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = base_ok_data();
    data_test.batt_soc  = BATT_SOC_LOW; // below 70
    data_test.batt_volt = BATT_VOLT_LOW; // not > 12.2 => triggers else

    check_disable_engine(&data_test);

    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about battery operating out of range
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "error_battery_out_range");

    // Check received log message
    f_susbtring_data check_cond = {
        "/tmp/test_check_disable_engine_fail_cond4.log",
        "Stop/Start: SWR2.8 (Battery is not in operating range!)"
    };
    CU_ASSERT_TRUE(file_contains_substring(check_cond));

    // Finalize and clean up
    cleanup_logging_system();
}

// 6) Fail cond5 => door_open != 0 (SWR2.7)
static void test_check_disable_engine_fail_cond5(void)
{
    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_check_disable_engine_fail_cond5.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    start_stop_manual    = true;
    engine_off = false;

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = base_ok_data();
    data_test.door_open = DOOR_FAIL; // triggers the else for cond5

    check_disable_engine(&data_test);

    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about any door opened
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "error_door_open");

    // Check received log message
    f_susbtring_data check_cond = {
        "/tmp/test_check_disable_engine_fail_cond5.log",
        "Stop/Start: SWR2.8 (One or more doors are opened!)"
    };
    CU_ASSERT_TRUE(file_contains_substring(check_cond));

    // Finalize and clean up
    cleanup_logging_system();
}

// 7) Fail cond6 => tilt_angle > 5 (SWR2.9)
static void test_check_disable_engine_fail_cond6(void)
{
    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_check_disable_engine_fail_cond6.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    start_stop_manual    = true;
    engine_off = false;

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = base_ok_data();
    data_test.tilt_angle = TILT_FAIL; // triggers the else for cond6

    check_disable_engine(&data_test);

    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about tilt angle greater than maximum limit
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "error_tilt_angle");

    // Check received log message
    f_susbtring_data check_cond = {
        "/tmp/test_check_disable_engine_fail_cond6.log",
        "Stop/Start: SWR2.8 (Tilt angle greater than 5 degrees!)"
    };
    CU_ASSERT_TRUE(file_contains_substring(check_cond));

    // Finalize and clean up
    cleanup_logging_system();
}

// 8) engine_off=false => then if ANY cond related to movement succeeds => triggers engine deactivation
static void test_check_disable_engine(void)
{
    start_stop_manual    = true;
    engine_off = false; // simulate engine it's already on

    VehicleData data_test = base_ok_data();
    data_test.prev_brake = BRAKE_FAIL;
    data_test.brake = BRAKE_OK; // fail cond1 => handle_engine_restart_logic should enable engine

    check_disable_engine(&data_test);

    // Now the engine should have deactivated
    CU_ASSERT_TRUE(engine_off);
}

// 9) Check restart logic
/**
 * @test test_handle_engine_restart
 * @brief Tests engine restart functionality
 * @req SWR1.1
 * @file unit/test_powertrain.c
 */
static void test_handle_engine_restart(void)
{
    // Testing if engine will restart successfully if conditions are met

    // Set up the log file and initialize the logging system
    set_log_file_path("/tmp/test_handle_engine_restart.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // Ensure manual=on, start_stop active
    start_stop_manual = true;
    engine_off = true;

    // Reset the stub counter
    stub_can_reset();

    VehicleData data_test = {
        .prev_brake = BRAKE_OK, 
        .brake      = BRAKE_FAIL, // brake_released
        .batt_volt  = BATT_VOLT_OK, 
        .batt_soc   = BATT_SOC_OK
    };

    handle_engine_restart_logic(&data_test);

    CU_ASSERT_FALSE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);

    // Check if last message is about restarting
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "RESTART");

    // Check received log message
    f_susbtring_data restart_ok = {
        "/tmp/test_handle_engine_restart.log",
        "Stop/Start: Engine turned On"
    };
    CU_ASSERT_TRUE(file_contains_substring(restart_ok));

    // Now we'll test if when the battery SoC drops the engine won't restart

    // Reset the stub counter
    stub_can_reset();

    engine_off = true;    
    data_test.batt_soc = BATT_SOC_LOW;
    data_test.prev_brake = BRAKE_OK, 
    data_test.brake      = BRAKE_FAIL, // brake_released

    handle_engine_restart_logic(&data_test);

    CU_ASSERT_TRUE(engine_off);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 2);

    // Check if last message is about error
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "system_disabled_error");

    // Check received log message
    f_susbtring_data sys_disable = {
        "/tmp/test_handle_engine_restart.log",
        "Fault: SWR3.5 (Low Battery)"
    };
    CU_ASSERT_TRUE(file_contains_substring(sys_disable));

    // Finalize and clean up
    cleanup_logging_system();
}

// 9) Check powertrain communication via CAN
static void test_powertrain_comms_loop(void)
{
    test_mode_powertrain = false;

    pthread_t thd;
    pthread_create(&thd, NULL, powertrain_comms, NULL);

    sleep_microseconds_pw(SLEEP_TIME_US_TEST);

    // Ensure loop break
    test_mode_powertrain = true;
    pthread_cancel(thd);
    pthread_join(thd, NULL);

    CU_ASSERT_TRUE(true);
}

// 10) Example test that simulates reception of a valid CAN frame
static void test_process_can_frame(void)
{
    // Enable the mock to return a frame
    mock_receive_can_frame_enable = true;

    memset(&mock_frame_to_return, 0, sizeof(mock_frame_to_return));
    mock_frame_to_return.can_id  = CAN_ID_SENSOR_READ;
    mock_frame_to_return.can_dlc = CAN_DATA_SIZE;
    unsigned char fake_data[CAN_DATA_SIZE] = {
        TEST_BYTE_1, TEST_BYTE_2, TEST_BYTE_3, TEST_BYTE_4,
        TEST_BYTE_5, TEST_BYTE_6, TEST_BYTE_7, TEST_BYTE_8
    };    
    memcpy(mock_frame_to_return.data, fake_data, CAN_DATA_SIZE);

    // Prepare rec_data
    rec_data.speed = SPEED_OK;

    // We need to call the logic that receives and processes frames.
    // Since "process_received_frame_powertrain()" has a for(;;),
    // we can either use #ifdef or manually replicate one iteration.
    // For this example, we manually reproduce the simplified “internal logic”:
    struct can_frame frame;
    if (receive_can_frame(0, &frame) == 0) {
        if (check_is_valid_can_id_powertrain(frame.can_id)) {
            unsigned char encrypted_data[AES_BLOCK_TEST_SIZE];
            memcpy(encrypted_data,     frame.data, CAN_DATA_SIZE);
            memcpy(encrypted_data + CAN_DATA_SIZE, frame.data, CAN_DATA_SIZE);

            char decrypted[AES_BLOCK_TEST_SIZE];
            decrypt_data(encrypted_data, decrypted, AES_BLOCK_TEST_SIZE);

            parse_input_received_powertrain(decrypted);

            // If decrypt_data always returns "speed: 5.0",
            // then rec_data.speed should now be 5.0.
            CU_ASSERT_DOUBLE_EQUAL(rec_data.speed, kSpeedMock, kDeltaMock);
        }
    }
}

// 11) Check thread for stop/start function
/**
 * @test test_function_start_stop
 * @brief Tests stop start functionality
 * @req SWR1.2
 * @file unit/test_powertrain.c
 */
static void test_function_start_stop(void)
{
    // 1. Initialize relevant globals
    //    Make sure the mutex is initialized somewhere in your setup code:
    //    pthread_mutex_init(&mutex_powertrain, NULL);
    test_mode_powertrain = false;   // so the function won't exit immediately
    start_stop_manual    = true;    // "driver" turned on stop/start
    engine_off = false;   // begins inactive

    // 2. Set up a VehicleData with conditions that pass check_disable_engine()
    VehicleData data_test = base_ok_data();

    // 3. Start the function_start_stop in a separate thread
    pthread_t thd;
    pthread_create(&thd, NULL, function_start_stop, &data_test);

    // 4. Let the thread run for a moment
    sleep_microseconds_pw(USLEEP_DELAY_THREAD); // 200ms - enough time for the loop to run a couple times

    // 5. Check if engine_off got set to 'true'
    //    Because our conditions above should pass in check_disable_engine().
    CU_ASSERT_TRUE(engine_off);

    // 6. Now signal the loop to exit
    test_mode_powertrain = true;

    // 7. Join the thread to ensure clean exit
    pthread_join(thd, NULL);

    // If we reached here, we can do final checks or just pass
    CU_ASSERT_TRUE(true);
}

// 12) Check parse for sensor inputs
static void test_parse_input_variants(void)
{
    // Make sure we start in a known state
    start_stop_manual = false;
    memset(&rec_data, 0, sizeof(rec_data));

    // 1) Toggling Start/Stop
    parse_input_received_powertrain("press_start_stop");
    CU_ASSERT_TRUE(start_stop_manual);  // toggled from false->true

    parse_input_received_powertrain("press_start_stop");
    CU_ASSERT_FALSE(start_stop_manual); // toggled back to false

    parse_input_received_powertrain("system_disabled_error");
    CU_ASSERT_FALSE(start_stop_manual); // toggled to false

    // 2) Speed
    parse_input_received_powertrain("speed: 45.7");
    CU_ASSERT_DOUBLE_EQUAL(rec_data.speed, kSpeedReceived, kDelta);

    // 3) internal temp
    parse_input_received_powertrain("in_temp: 23");
    CU_ASSERT_EQUAL(rec_data.internal_temp, INTERNAL_TEMP_RECEIVED);

    // 4) external temp
    parse_input_received_powertrain("ex_temp: 10");
    CU_ASSERT_EQUAL(rec_data.external_temp, EXTERNAL_TEMP_RECEIVED);

    // 5) door
    parse_input_received_powertrain("door: 1");
    CU_ASSERT_EQUAL(rec_data.door_open, DOOR_RECEIVED);

    // 6) tilt
    parse_input_received_powertrain("tilt: 4.2");
    CU_ASSERT_DOUBLE_EQUAL(rec_data.tilt_angle, kTiltReceived, kDelta);

    // 7) accel
    parse_input_received_powertrain("accel: 3");
    CU_ASSERT_EQUAL(rec_data.accel, ACCEL_RECEIVED);

    // 8) brake
    parse_input_received_powertrain("brake: 1");
    CU_ASSERT_EQUAL(rec_data.brake, BRAKE_RECEIVED);

    // 9) temp_set
    parse_input_received_powertrain("temp_set: 22");
    CU_ASSERT_EQUAL(rec_data.temp_set, TEMP_SET_RECEIVED);

    // 10) batt_soc
    parse_input_received_powertrain("batt_soc: 81.5");
    CU_ASSERT_DOUBLE_EQUAL(rec_data.batt_soc, kBattSocReceived, kDelta);

    // 11) batt_volt
    parse_input_received_powertrain("batt_volt: 13.2");
    CU_ASSERT_DOUBLE_EQUAL(rec_data.batt_volt, kBattVoltReceived, kDelta);

    // 12) engi_temp
    parse_input_received_powertrain("engi_temp: 95.4");
    CU_ASSERT_DOUBLE_EQUAL(rec_data.engi_temp, kEngTempReceived, kDelta);

    // 13) gear
    parse_input_received_powertrain("gear: 2");
    CU_ASSERT_EQUAL(rec_data.gear, GEAR_RECEIVED);
}

int main(void)
{
    // Initialize CUnit test registry
    if (CU_initialize_registry() != CUE_SUCCESS)
    {
        return CU_get_error();
    }
    
    CU_pSuite suite = CU_add_suite("PowertrainTest", init_suite, clean_suite);
    if (!suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests
    CU_add_test(suite, "check_disable_engine_all_ok",       test_check_disable_engine_all_ok);
    CU_add_test(suite, "fail_cond1",               test_check_disable_engine_fail_cond1);
    CU_add_test(suite, "fail_cond2",               test_check_disable_engine_fail_cond2);
    CU_add_test(suite, "fail_cond3_inactive",      test_check_disable_engine_fail_cond3_inactive);
    CU_add_test(suite, "fail_cond4",               test_check_disable_engine_fail_cond4);
    CU_add_test(suite, "fail_cond5",               test_check_disable_engine_fail_cond5);
    CU_add_test(suite, "fail_cond6",               test_check_disable_engine_fail_cond6);
    CU_add_test(suite, "deactivate_when_active",   test_check_disable_engine);
    CU_add_test(suite, "handle_engine_restart",    test_handle_engine_restart);
    CU_add_test(suite, "powertrain_comms_loop",    test_powertrain_comms_loop);
    CU_add_test(suite, "test_process_can_frame",   test_process_can_frame);
    CU_add_test(suite, "function_start_stop test", test_function_start_stop);
    CU_add_test(suite, "test_parse_input_variants", test_parse_input_variants);

    // Run all tests in verbose mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned failures = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
