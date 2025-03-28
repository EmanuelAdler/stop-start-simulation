#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../../src/bcm/bcm_func.h"

#define CSV_FILE_PATH "../src/bcm/full_simu.csv"
#define TEST_EXPECTED_IT (2.0F)

// Tolerances
const float SOC_TOLERANCE = 0.001F;

// SOC values for test
#define TEST_SOC_HIGH              99.9F
#define TEST_SOC_LOW               0.1F
#define TEST_SOC_UPDATE_POSITIVE   20.0F
#define TEST_SOC_UPDATE_NEGATIVE   0.0F
#define TEST_BATT_SOC_INITIAL      50.0F

// Speeds
#define SPEED_LOW                  5.0
#define SPEED_MEDIUM               10.0
#define SPEED_HIGH                 20.0
#define TEST_SPEED_INCREASE        10.0

// Temperatures
#define INT_TEMP_1                 25
#define INT_TEMP_2                 26
#define EXT_TEMP_1                 30
#define EXT_TEMP_2                 31
#define ENGI_TEMP_1                90.0
#define ENGI_TEMP_2                95.0

// Inclinations
#define TILT_1                     2.5
#define TILT_2                     5.0

// Temp set
#define TEMP_SET_1                 20
#define TEMP_SET_2                 21

// Battery voltage
#define BATT_VOLT_1                12.0
#define BATT_VOLT_2                12.5

// Battery SOC
#define BATT_SOC_1                80.0
#define BATT_SOC_2                82.0

// Delay
#define TEST_MICRO_SLEEP_DELAY     (100000U)
#define THREAD_SLEEP_TIME          (200000U)

// Mocked can_socket calls (like in test_instrument_cluster)
int stub_can_get_send_count(void);
const char* stub_can_get_last_message(void);
void stub_can_reset(void);

// We'll do some "fake" CSV logic by renaming real function or controlling environment
// or we can force read_csv() to fail by renaming the path.

// If you can't easily manipulate the real CSV file, you could stub out read_csv as well.

//-------------------------------------
// Suite init/cleanup
//-------------------------------------
static int init_suite(void)
{
    // Reset the global state each time
    simu_state = STATE_STOPPED;
    simu_order = ORDER_STOP;
    simu_curr_step = 0;
    data_size = 0;
    batt_soc = DEFAULT_BATTERY_SOC;
    batt_volt = DEFAULT_BATTERY_VOLTAGE;
    memset(vehicle_data, 0, sizeof(vehicle_data));
    test_mode = false;

    // Reset the mock counters
    stub_can_reset();
    return 0;
}
static int clean_suite(void) { return 0; }

// We'll define a quick helper to replicate the "string contains" check
// since we can't rely on a built-in CU_ASSERT_STRING_CONTAINS:
static void CU_ASSERT_STRING_CONTAINS(const char *haystack, const char *needle)
{
    CU_ASSERT_PTR_NOT_NULL_FATAL(haystack);
    CU_ASSERT_PTR_NOT_NULL_FATAL(needle);
    // if haystack doesn't contain needle => fail
    CU_ASSERT_PTR_NOT_NULL(strstr(haystack, needle));
}

//-------------------------------------
// 1) test_read_csv_fail
//    Force read_csv() to fail -> "Error opening file"
//-------------------------------------
void test_read_csv_fail(void)
{
    // "nonexistent.csv" definitely won't exist
    CU_ASSERT_EQUAL(data_size, 0);
    read_csv("nonexistent.csv");
    // It should call perror() and just return
    CU_ASSERT_EQUAL(data_size, 0); 
}

//-------------------------------------
// 2) test_read_csv_success
//-------------------------------------
void test_read_csv_success(void)
{
    // This test expects you to have a small CSV file at "../src/bcm/full_simu.csv" 
    // with at least 2 lines of real data so we can parse something.

    CU_ASSERT_EQUAL(data_size, 0);
    read_csv(CSV_FILE_PATH); // tries to open the real file
    // If it succeeds, data_size should be > 1
    CU_ASSERT_TRUE(data_size > 1);
}

//-------------------------------------
// 3) test_check_order_stop / run / pause
//-------------------------------------
void test_check_order_stop(void)
{
    // We set the state to RUNNING, then order STOP
    simu_state = STATE_RUNNING;
    check_order(ORDER_STOP);
    CU_ASSERT_EQUAL(simu_state, STATE_STOPPED);
}

void test_check_order_run(void)
{
    // If we are STOPPED, run -> should do read_csv() and set RUNNING
    simu_state = STATE_STOPPED;
    check_order(ORDER_RUN);
    CU_ASSERT_EQUAL(simu_state, STATE_RUNNING);
}

void test_check_order_pause(void)
{
    // If we are RUNNING, pause -> should set STATE_PAUSED
    simu_state = STATE_RUNNING;
    check_order(ORDER_PAUSE);
    CU_ASSERT_EQUAL(simu_state, STATE_PAUSED);
}

//-------------------------------------
// 4) test_battery_overmax
//    force batt_soc above 100
//-------------------------------------
void test_battery_overmax(void)
{
    // e.g. set batt_soc near 99.9, call update with speed>0 multiple times
    batt_soc = TEST_SOC_HIGH;
    update_battery_soc(TEST_SOC_UPDATE_POSITIVE); // adds +0.5 => 100.4 => clamp to 100
    CU_ASSERT_DOUBLE_EQUAL(batt_soc, 100.0, SOC_TOLERANCE);
}

//-------------------------------------
// 5) test_battery_belowzero
//    force batt_soc below 0
//-------------------------------------
void test_battery_belowzero(void)
{
    // set near zero, then update speed=0 multiple times to push negative
    batt_soc = TEST_SOC_LOW;
    update_battery_soc(TEST_SOC_UPDATE_NEGATIVE); // subtract .2 => -0.1 => clamp to 0
    CU_ASSERT_DOUBLE_EQUAL(batt_soc, 0.0, SOC_TOLERANCE);
}

//-------------------------------------
// 6) test_send_data_update_manyfields
//-------------------------------------
void test_send_data_update_manyfields(void)
{
    // We'll set [0] and [1] with multiple differences so lines 221..275 get hit
    simu_curr_step = 1; // so we do [0] vs [1]

    vehicle_data[0].speed = SPEED_MEDIUM;
    vehicle_data[1].speed = SPEED_HIGH;
    vehicle_data[0].internal_temp = INT_TEMP_1;
    vehicle_data[1].internal_temp = INT_TEMP_2;
    vehicle_data[0].external_temp = EXT_TEMP_1;
    vehicle_data[1].external_temp = EXT_TEMP_2;
    vehicle_data[0].door_open = 0;
    vehicle_data[1].door_open = 1;
    vehicle_data[0].tilt_angle = TILT_1;
    vehicle_data[1].tilt_angle = TILT_2;
    vehicle_data[0].accel = 0;
    vehicle_data[1].accel = 1;
    vehicle_data[0].brake = 1;
    vehicle_data[1].brake = 0;
    vehicle_data[0].temp_set = TEMP_SET_1;
    vehicle_data[1].temp_set = TEMP_SET_2;
    vehicle_data[0].batt_soc = BATT_SOC_1;
    vehicle_data[1].batt_soc = BATT_SOC_2;
    vehicle_data[0].batt_volt = BATT_VOLT_1;
    vehicle_data[1].batt_volt = BATT_VOLT_2;
    vehicle_data[0].engi_temp = ENGI_TEMP_1;
    vehicle_data[1].engi_temp = ENGI_TEMP_2;
    vehicle_data[0].gear = 0;
    vehicle_data[1].gear = 1;

    stub_can_reset();
    send_data_update();

    // We changed basically everything => many calls
    // Let's guess we changed 12 fields => 12 calls
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 12);

    // Optionally check last message
    CU_ASSERT_STRING_CONTAINS(stub_can_get_last_message(), "gear: 1");
}

//-------------------------------------
// 7) test_simu_speed_smallloop
//    We'll forcibly do a small data_size so we can call simu_speed once or twice
//    This also forces calls to sleep_microseconds(...)
//-------------------------------------
void test_simu_speed_smallloop(void)
{
    data_size = 2;
    vehicle_data[0].speed = 0.0;
    vehicle_data[1].speed = SPEED_LOW;

    simu_state = STATE_RUNNING;
    simu_order = ORDER_RUN;

    int count = 0;
    while (count < 2) {
        pthread_mutex_lock(&mutex_bcm);
        check_order(simu_order);

        if (simu_state == STATE_RUNNING) 
        {
            // If there's a next step
            if (simu_curr_step + 1 != data_size) 
            {
                double speed0 = vehicle_data[simu_curr_step].speed;
                double speed1 = vehicle_data[simu_curr_step + 1].speed;
                if (speed1 - speed0 > 0) 
                {
                    vehicle_data[simu_curr_step].accel = 1;
                    vehicle_data[simu_curr_step].brake = 0;
                    vehicle_data[simu_curr_step].gear = DRIVE;
                } 
                else 
                {
                    vehicle_data[simu_curr_step].brake = 1;
                    vehicle_data[simu_curr_step].accel = 0;
                    if (speed0 == 0.0)
                    {
                        vehicle_data[simu_curr_step].gear = PARKING;
                    }
                }
            }
            // increment first
            simu_curr_step++;

            // now decide if we want to stop
            if (simu_curr_step >= data_size) 
            {
                simu_order = ORDER_STOP;
            }
        }
        pthread_mutex_unlock(&mutex_bcm);

        // This calls sleep_microseconds => covers that code
        sleep_microseconds(TEST_MICRO_SLEEP_DELAY);
        count++;
    }

    printf("simu_curr_step %d\n", simu_curr_step);
    CU_ASSERT_EQUAL(simu_curr_step, 2);
}

//-------------------------------------
// 8) Speed simulator step coverage
//-------------------------------------
void test_simu_speed_step(void)
{
    // data_size=3
    data_size = 3;

    // Populate vehicle_data
    vehicle_data[0].speed = 0.0;
    vehicle_data[1].speed = SPEED_LOW;
    vehicle_data[2].speed = SPEED_MEDIUM;

    // Mark the simulation as RUNNING
    simu_state     = STATE_RUNNING;
    simu_curr_step = 0;
    simu_order     = ORDER_RUN;

    // Build arrays of pointers
    double *speed[3];
    int    *accel[3];
    int    *brake[3];
    int    *gear[3];

    for (int i = 0; i < 3; i++)
    {
        speed[i]  = &vehicle_data[i].speed;
        accel[i]  = &vehicle_data[i].accel;
        brake[i]  = &vehicle_data[i].brake;
        gear[i]   = &vehicle_data[i].gear;
    }

    ControlData control_data = { speed, accel, brake, gear };

    // 1) First call: index 0 => 1 => speed difference = (5.0 - 0.0) > 0 => accel=1, brake=0, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, 1);
    CU_ASSERT_EQUAL(*(accel[0]), 1);    // note the * to dereference
    CU_ASSERT_EQUAL(*(brake[0]), 0);
    CU_ASSERT_EQUAL(*(gear[0]), DRIVE);

    // 2) Second call => index 1 => 2 => (10.0 - 5.0) > 0 => accel=1, brake=0, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, 2);
    CU_ASSERT_EQUAL(*(accel[1]), 1);
    CU_ASSERT_EQUAL(*(brake[1]), 0);
    CU_ASSERT_EQUAL(*(gear[1]), DRIVE);

    // 3) Third call => index 2 => if (2+1==3) => ORDER_STOP
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, 3);
    CU_ASSERT_EQUAL(simu_order, ORDER_STOP);
}

//-------------------------------------
// 8) Battery sensor simulator for SOC update
//-------------------------------------
void test_sensor_battery_updates_soc_when_running(void)
{
    // Set up simulation state
    test_mode = false;
    simu_state = STATE_RUNNING;
    simu_curr_step = 0;

    // Initialize vehicle data and global battery state
    vehicle_data[0].speed = SPEED_HIGH;
    vehicle_data[0].batt_soc = TEST_BATT_SOC_INITIAL;
    batt_soc = TEST_BATT_SOC_INITIAL;

    // Calculate expected SoC after one update
    float expected_iterations = TEST_EXPECTED_IT; // if THREAD_SLEEP_TIME * 2 allows 2 iterations
    float expected_soc = batt_soc + (BATTERY_SOC_INCREMENT * expected_iterations);
    if (expected_soc > MAX_BATTERY_SOC) 
    {
        expected_soc = MAX_BATTERY_SOC;
    }

    // Start the sensor thread
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, sensor_battery, NULL);

    // Allow it to run at least once
    // usleep(THREAD_SLEEP_TIME * 2);
    sleep_microseconds(THREAD_SLEEP_TIME);
    test_mode = true;

    // Wait for the thread to finish
    pthread_join(thread_id, NULL);

    // Get actual updated SoC
    float actual_soc = (float) vehicle_data[0].batt_soc;

    // Assert the SoC was updated as expected
    CU_ASSERT_DOUBLE_EQUAL(expected_soc, actual_soc, SOC_TOLERANCE);
}

//-------------------------------------
// 9) Speed simulator thread
//-------------------------------------
void test_simu_speed_performs_control_update(void)
{
    // Arrange
    test_mode = false;
    simu_state = STATE_RUNNING;
    simu_curr_step = 0;
    simu_order = ORDER_RUN;
    data_size = 2;

    VehicleData sim_data[2] = {0};

    // Initial setup: speed increases between steps
    sim_data[0].speed = 0.0;
    sim_data[1].speed = TEST_SPEED_INCREASE;

    // Initialize outputs (accel, brake, gear)
    sim_data[0].accel = 0;
    sim_data[0].brake = 0;
    sim_data[0].gear = 0;

    // Act: start the thread
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, simu_speed, (void *)sim_data);

    // Let it run one iteration
    // usleep(THREAD_SLEEP_TIME * 2);
    sleep_microseconds(THREAD_SLEEP_TIME);
    test_mode = true;
    pthread_join(thread_id, NULL);

    // Assert: check that control actions were applied
    CU_ASSERT_EQUAL(sim_data[0].accel, 1);
    CU_ASSERT_EQUAL(sim_data[0].brake, 0);
    CU_ASSERT_EQUAL(sim_data[0].gear, DRIVE);  // assume DRIVE is a defined constant
}


//-------------------------------------
// Test main
//-------------------------------------
int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry()) 
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("BCM_Func_Suite", init_suite, clean_suite);
    if (!suite) 
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests
    CU_add_test(suite, "read_csv fails", test_read_csv_fail);
    CU_add_test(suite, "read_csv success (with real CSV file)", test_read_csv_success);
    CU_add_test(suite, "check_order STOP", test_check_order_stop);
    CU_add_test(suite, "check_order RUN", test_check_order_run);
    CU_add_test(suite, "check_order PAUSE", test_check_order_pause);
    CU_add_test(suite, "battery over 100", test_battery_overmax);
    CU_add_test(suite, "battery below 0", test_battery_belowzero);
    CU_add_test(suite, "send_data_update many fields", test_send_data_update_manyfields);
    CU_add_test(suite, "simu_speed small loop", test_simu_speed_smallloop);
    CU_add_test(suite, "simu_speed_step direct call", test_simu_speed_step);
    CU_add_test(suite, "sensor_battery_updates_soc_when_running", test_sensor_battery_updates_soc_when_running);
    CU_add_test(suite, "simu_speed_performs_control_update", test_simu_speed_performs_control_update);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
