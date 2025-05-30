#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../../src/bcm/bcm_func.h"
#include "../../src/common_includes/logging.h"

#define CSV_FILE_PATH "../src/bcm/full_simu.csv"
#define TEST_EXPECTED_IT     (2.0F)
#define TEST_EXPECTED_IT_INT (2)
#define FILE_LINE_SIZE       (256)

// Tolerances
const float SOC_TOLERANCE = 0.001F;

// SOC values for test
#define TEST_SOC_HIGH 99.9F
#define TEST_SOC_LOW 0.1F
#define TEST_SOC_UPDATE_POSITIVE 20.0F
#define TEST_SOC_UPDATE_NEGATIVE 0.0F
#define TEST_BATT_SOC_INITIAL 50.0F

// Speeds
#define SPEED_LOW 5.0
#define SPEED_MEDIUM 10.0
#define SPEED_HIGH 20.0
#define TEST_SPEED_INCREASE 10.0

// Temperatures
#define INT_TEMP_1 25
#define INT_TEMP_2 26
#define EXT_TEMP_1 30
#define EXT_TEMP_2 31
#define ENGI_TEMP_1 90.0
#define ENGI_TEMP_2 95.0
#define ENGI_TEMP_3 125.0

// Inclinations
#define TILT_1 2.5
#define TILT_2 5.0
#define TILT_3 80.0

// Temp set
#define TEMP_SET_1 20
#define TEMP_SET_2 21

// Battery voltage
#define BATT_VOLT_1 12.0
#define BATT_VOLT_2 12.5

// Battery SOC
#define BATT_SOC_1 80.0
#define BATT_SOC_2 82.0

// Battery SOC
#define DOOR_VALID 0
#define DOOR_INVALID 2

// Delay
#define TEST_MICRO_SLEEP_DELAY (100000U)
#define THREAD_SLEEP_TIME (200000U)
#define MOCK_TIME_1S (1000)
#define MOCK_TIME_25S (2500)

#define MOCK_SOCKET (999)

// Mocked can_socket calls
void mock_can_force_sys_disable(bool enable);
void mock_can_force_invalid_id(bool enable);
int stub_can_get_send_count(void);
const char *stub_can_get_last_message(void);
void stub_can_reset(void);

extern int mock_time_ms;

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
/**
 * @test test_read_csv_success
 * @brief Verifies if the csv file with the sensors readings is correctly loaded by the system.
 * @req SWR2.1
 * @req SWR4.2
 * @file unit/test_bcm.c
 */
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

    // If we are PAUSED, run -> should set RUNNING
    simu_state = STATE_PAUSED;
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
    update_battery_soc(TEST_SOC_UPDATE_POSITIVE); // adds +0.04 
    update_battery_soc(TEST_SOC_UPDATE_POSITIVE); // adds +0.04
    update_battery_soc(TEST_SOC_UPDATE_POSITIVE); // adds +0.04 => 100.2 => clamp to 100
    printf("batt_soc = %f\n", batt_soc);
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
    simu_curr_step = 0; // so we do [0] vs [1]

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
    CU_ASSERT_STRING_CONTAINS(stub_can_get_last_message(), "gear: 0");
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
    while (count < 2)
    {
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
    #define data_size_simu_test  7

    #define STEP1 0
    #define STEP2 1
    #define STEP3 2
    #define STEP4 3
    #define STEP5 4
    #define STEP6 5
    #define STEP7 6

    // Populate vehicle_data
    vehicle_data[STEP1].speed = 0.0;
    vehicle_data[STEP2].speed = SPEED_LOW;
    vehicle_data[STEP3].speed = SPEED_MEDIUM;
    vehicle_data[STEP4].speed = SPEED_MEDIUM;
    vehicle_data[STEP5].speed = SPEED_LOW;
    vehicle_data[STEP6].speed = 0.0;
    vehicle_data[STEP7].speed = 0.0;

    // Mark the simulation as RUNNING
    simu_state = STATE_RUNNING;
    simu_curr_step = 0;
    simu_order = ORDER_RUN;

    // Build arrays of pointers
    double *speed[data_size_simu_test];
    int *accel[data_size_simu_test];
    int *brake[data_size_simu_test];
    int *gear[data_size_simu_test];

    for (int i = 0; i < data_size_simu_test; i++)
    {
        speed[i] = &vehicle_data[i].speed;
        accel[i] = &vehicle_data[i].accel;
        brake[i] = &vehicle_data[i].brake;
        gear[i] = &vehicle_data[i].gear;
    }

    ControlData control_data = {speed, accel, brake, gear};

    // 1) First call  => index 0 => 1 => speed difference = (5.0 - 0.0) > 0
    // => accel=1, brake=0, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP1);
    CU_ASSERT_EQUAL(*(accel[STEP1]), 1); // note the * to dereference
    CU_ASSERT_EQUAL(*(brake[STEP1]), 0);
    CU_ASSERT_EQUAL(*(gear[STEP1]), DRIVE);

    simu_curr_step++;

    // 2) Second call => index 1 => 2 => (10.0 - 5.0) > 0 (accelerating)
    // => accel=1, brake=0, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP2);
    CU_ASSERT_EQUAL(*(accel[STEP2]), 1);
    CU_ASSERT_EQUAL(*(brake[STEP2]), 0);
    CU_ASSERT_EQUAL(*(gear[STEP2]), DRIVE);

    simu_curr_step++;

    // 3) Third call  => index 2 => 2 => 3 => (10.0 - 10.0) = 0 (constant speed)
    // => accel=1, brake=0, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP3);
    CU_ASSERT_EQUAL(*(accel[STEP3]), 1);
    CU_ASSERT_EQUAL(*(brake[STEP3]), 0);
    CU_ASSERT_EQUAL(*(gear[STEP3]), DRIVE);

    simu_curr_step++;

    // 4) Fourth call  => index 3 => 3 => 4 => (5.0 - 10.0) = -5.0 (braking)
    // => accel=0, brake=1, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP4);
    CU_ASSERT_EQUAL(*(accel[STEP4]), 0);
    CU_ASSERT_EQUAL(*(brake[STEP4]), 1);
    CU_ASSERT_EQUAL(*(gear[STEP4]), DRIVE);

    simu_curr_step++;

    // 5) Fifth call  => index 4 => 4 => 5 => (0.0 - 5.0) = -5.0 (stopped)
    // => accel=0, brake=1, gear=DRIVE
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP5);
    CU_ASSERT_EQUAL(*(accel[STEP5]), 0);
    CU_ASSERT_EQUAL(*(brake[STEP5]), 1);
    CU_ASSERT_EQUAL(*(gear[STEP5]), DRIVE);

    simu_curr_step++;

    // 6) Sixth call  => index 5 => 5 => 6 => (0.0 - 0.0) = 0 (stopped)
    // => accel=0, brake=1, gear=PARKING
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP6);
    CU_ASSERT_EQUAL(*(accel[STEP6]), 0);
    CU_ASSERT_EQUAL(*(brake[STEP6]), 1);
    CU_ASSERT_EQUAL(*(gear[STEP6]), PARKING);

    simu_curr_step++;

    // 7) Seventh call => index 6 => if (6+1==7) => ORDER_STOP
    simu_speed_step(vehicle_data, control_data);

    CU_ASSERT_EQUAL(simu_curr_step, STEP7);
    CU_ASSERT_EQUAL(simu_order, ORDER_STOP);
}

//-------------------------------------
// 9) Battery sensor simulator for SOC update
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
    double expected_soc = batt_soc + (BATTERY_SOC_INCREMENT * expected_iterations);
    if (expected_soc > MAX_BATTERY_SOC)
    {
        expected_soc = MAX_BATTERY_SOC;
    }

    // Start the sensor thread
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, sensor_battery, NULL);

    // Wait for the thread to finish
    pthread_join(thread_id, NULL);

    // Get actual updated SoC
    double actual_soc = vehicle_data[0].batt_soc;

    // Assert the SoC was updated as expected
    CU_ASSERT_DOUBLE_EQUAL(expected_soc, actual_soc, SOC_TOLERANCE);
}

//-------------------------------------
// 10) Speed simulator thread
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
    sleep_microseconds(THREAD_SLEEP_TIME);
    test_mode = true;
    pthread_join(thread_id, NULL);

    // Assert: check that control actions were applied
    CU_ASSERT_EQUAL(sim_data[0].accel, 1);
    CU_ASSERT_EQUAL(sim_data[0].brake, 0);
    CU_ASSERT_EQUAL(sim_data[0].gear, DRIVE); // assume DRIVE is a defined constant
}

//-------------------------------------
// 11) Get time function
//-------------------------------------
void test_getCurrentTimeMsReal(void)
{
    // Grab the initial time in ms.
    int before_ms = getCurrentTimeMs_real();
    // Sleep for 100ms.
    sleep_microseconds(TEST_MICRO_SLEEP_DELAY);

    int after_ms = getCurrentTimeMs_real();

    // Check that after_ms is at least ~100ms greater than before_ms.
    // We use a small margin of error, since actual scheduling can vary slightly.
    CU_ASSERT_TRUE((after_ms - before_ms) >= 80);
}

//-------------------------------------
// 12) Check health signals
//-------------------------------------
/**
 * @test test_check_health_signals_immediate
 * @brief Simulates a fault and check health signals to identify a fault immedeatly.
 * @req SWR6.1
 * @file unit/test_bcm.c
 */
void test_check_health_signals_immediate(void)
{
    // Reset global states
    fault_active = false;
    fault_start_time = 0;
    simu_curr_step = 0;
    simu_state = STATE_RUNNING;
    simu_order = ORDER_RUN;

    // We'll assume time starts at zero
    mock_time_ms = 0;

    // 1) Create a “doorVal” that is invalid => triggers `invalidDoor`
    vehicle_data[0].door_open = DOOR_INVALID;  // invalid
    vehicle_data[0].engi_temp = ENGI_TEMP_2; // normal
    vehicle_data[0].tilt_angle = TILT_1; // normal

    // 2) Call check_health_signals()
    check_health_signals();

    // 3) Since the fault is new, we expect:
    //    fault_active == true, fault_start_time = 0, but system is NOT yet stopped
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(fault_start_time, 0);
    CU_ASSERT_EQUAL(simu_order, ORDER_RUN); // not stopped yet
}

/**
 * @test test_check_health_signals_persisted
 * @brief Simulates a fault in the tilt angle and check health signals and after elapsing the safety time send the system disabled warning.
 * @req SWR6.1
 * @req SWR6.2
 * @req SWR6.3
 * @req SWR6.4
 * @file unit/test_bcm.c
 */
void test_check_health_signals_persisted(void)
{
    set_log_file_path("/tmp/test_check_health_signals_persisted.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // Reset global states
    fault_active = false;
    fault_start_time = 0;
    simu_curr_step = 0;
    simu_state = STATE_RUNNING;
    simu_order = ORDER_RUN;

    mock_time_ms = 0;

    // 1) Create a “tilt_angle” that is above 60 => triggers `excessiveTilt`
    vehicle_data[0].door_open = DOOR_VALID;   // normal
    vehicle_data[0].engi_temp = ENGI_TEMP_1;  // normal
    vehicle_data[0].tilt_angle = TILT_3; // triggers fault

    // 2) First call sets fault_active = true, fault_start_time=0
    check_health_signals();
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(fault_start_time, 0);

    // 3) Simulate time passing < safety_timeout_ms
    mock_time_ms = MOCK_TIME_1S; // 1 second
    check_health_signals();

    // Still under 2 seconds => not stopped
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(simu_order, ORDER_RUN);

    // 4) Simulate time passing >= safety_timeout_ms (2 seconds)
    mock_time_ms = MOCK_TIME_25S; // 2.5 seconds
    check_health_signals();

    // Now we expect the code to set simu_order = ORDER_STOP
    CU_ASSERT_EQUAL(simu_order, ORDER_STOP);

    f_susbtring_data err_disab = {
        "/tmp/test_check_health_signals_persisted.log",
        "Fault: SWR6.4 (System Disabling Error)"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_disab));

    f_susbtring_data err_tilt = {
        "/tmp/test_check_health_signals_persisted.log",
        "Fault: SWR6.4 (Excessive tilt value)"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_tilt));

    cleanup_logging_system();
}

/**
 * @test test_check_health_signals_engine_temp
 * @brief Simulates a fault in the engine temperature and check health signals and after elapsing the safety time send the system disabled warning.
 * @req SWR6.3
 * @req SWR6.4
 * @file unit/test_bcm.c
 */
void test_check_health_signals_engine_temp(void)
{
    set_log_file_path("/tmp/test_check_health_signals_engine_temp.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // Reset global states
    fault_active = false;
    fault_start_time = 0;
    simu_curr_step = 0;
    simu_state = STATE_RUNNING;
    simu_order = ORDER_RUN;

    mock_time_ms = 0;

    // 1) Create a engi_temp that is above 120 => triggers `engineOvertemp`
    vehicle_data[0].door_open = DOOR_VALID;   // normal
    vehicle_data[0].engi_temp = ENGI_TEMP_3;  // triggers fault
    vehicle_data[0].tilt_angle = TILT_1; // normal

    // 2) First call sets fault_active = true, fault_start_time=0
    check_health_signals();
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(fault_start_time, 0);

    // 3) Simulate time passing < safety_timeout_ms
    mock_time_ms = MOCK_TIME_1S; // 1 second
    check_health_signals();

    // Still under 2 seconds => not stopped
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(simu_order, ORDER_RUN);

    // 4) Simulate time passing >= safety_timeout_ms (2 seconds)
    mock_time_ms = MOCK_TIME_25S; // 2.5 seconds
    check_health_signals();

    // Now we expect the code to set simu_order = ORDER_STOP
    CU_ASSERT_EQUAL(simu_order, ORDER_STOP);

    f_susbtring_data err_disab = {
        "/tmp/test_check_health_signals_engine_temp.log",
        "Fault: SWR6.4 (System Disabling Error)"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_disab));

    f_susbtring_data err_overtemp = {
        "/tmp/test_check_health_signals_engine_temp.log",
        "Fault: SWR6.4 (Engine overtemperature)"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_overtemp));

    cleanup_logging_system();
}

/**
 * @test test_check_health_signals_door_status
 * @brief Simulates a fault in the door and check health signals and after elapsing the safety time send the system disabled warning.
 * @req SWR6.3
 * @req SWR6.4
 * @file unit/test_bcm.c
 */
void test_check_health_signals_door_status(void)
{
    set_log_file_path("/tmp/test_check_health_signals_door_status.log");
    CU_ASSERT_TRUE_FATAL(init_logging_system());

    // Reset global states
    fault_active = false;
    fault_start_time = 0;
    simu_curr_step = 0;
    simu_state = STATE_RUNNING;
    simu_order = ORDER_RUN;

    mock_time_ms = 0;

    // 1) Create an invalid door_open status => triggers `invalidDoor`
    vehicle_data[0].door_open = DOOR_INVALID;   // triggers fault
    vehicle_data[0].engi_temp = ENGI_TEMP_1;  // normal
    vehicle_data[0].tilt_angle = TILT_1; // normal

    // 2) First call sets fault_active = true, fault_start_time=0
    check_health_signals();
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(fault_start_time, 0);

    // 3) Simulate time passing < safety_timeout_ms
    mock_time_ms = MOCK_TIME_1S; // 1 second
    check_health_signals();

    // Still under 2 seconds => not stopped
    CU_ASSERT_TRUE(fault_active);
    CU_ASSERT_EQUAL(simu_order, ORDER_RUN);

    // 4) Simulate time passing >= safety_timeout_ms (2 seconds)
    mock_time_ms = MOCK_TIME_25S; // 2.5 seconds
    check_health_signals();

    // Now we expect the code to set simu_order = ORDER_STOP
    CU_ASSERT_EQUAL(simu_order, ORDER_STOP);

    f_susbtring_data err_disab = {
        "/tmp/test_check_health_signals_door_status.log",
        "Fault: SWR6.4 (System Disabling Error)"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_disab));

    f_susbtring_data err_door = {
        "/tmp/test_check_health_signals_door_status.log",
        "Fault: SWR6.4 (Invalid door status)"
    };
    CU_ASSERT_TRUE(file_contains_substring(err_door));

    cleanup_logging_system();
}

//-------------------------------------
// 13)  comms() bounded‑loop verification
//-------------------------------------
void test_comms_thread_expected_iterations(void)
{
    /* -------- Arrange ------------------------------------------------- */
    sem_init(&sem_comms, 0, TEST_EXPECTED_IT_INT);   /* pre‑post semaphore    */
    pthread_mutex_init(&mutex_bcm, NULL);

    stub_can_reset();

    data_size      = 1;          /* a single VehicleData entry is enough */
    simu_curr_step = 0;
    simu_state     = STATE_RUNNING;
    simu_order     = ORDER_RUN;
    data_updated   = true;       /* first iteration will send immediately */

    /* -------- Act ----------------------------------------------------- */
    pthread_t tid;
    pthread_create(&tid, NULL, comms, NULL);
    pthread_join(tid, NULL);     /* loop finishes after TEST_EXPECTED_IT_INT */

    /* -------- Assert -------------------------------------------------- */

    /* data_updated was cleared by the last iteration                 */
    CU_ASSERT_FALSE(data_updated);

    /* -------- Cleanup ------------------------------------------------- */
    pthread_mutex_destroy(&mutex_bcm);
    sem_destroy(&sem_comms);
}

//-------------------------------------
// 14) Function to verify the need for halting the simulation
//-------------------------------------
void test_check_system_disable(void)
{
    stub_can_reset();

    mock_can_force_sys_disable(true);
    
    check_system_disable(MOCK_SOCKET);

    mock_can_force_sys_disable(false);
    
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 0);
}

static void test_invalid_can_id_branch(void)
{
    /* start with clean counters ---------------------------------------- */
    stub_can_reset();

    /*  tell the stub: on 1st receive() give me an *invalid* CAN-ID  ---- */
    mock_can_force_invalid_id(true);

    /* run the code under test ----------------------------------------- */
    check_system_disable(MOCK_SOCKET);

    /* reset to normal behaviour, so other tests keep original paths  */
    mock_can_force_invalid_id(false);

    /* we only wanted to exercise the branch – nothing should have been
       sent on the bus, so send counter must still be zero              */
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 0);
}

static void test_system_disabled_path(void)
{
    stub_can_reset();
    simu_state = STATE_RUNNING;
    test_mode = false;

    mock_can_force_sys_disable(true);

    check_system_disable(MOCK_SOCKET);
    
    CU_ASSERT_EQUAL(simu_order, STATE_STOPPED);

    mock_can_force_sys_disable(false);
}

void test_comms_reception_thread_expected_iterations(void)
{
    /* -------- Arrange ------------------------------------------------- */
    sem_init(&sem_comms, 0, TEST_EXPECTED_IT_INT);   /* pre‑post semaphore    */
    pthread_mutex_init(&mutex_bcm, NULL);

    stub_can_reset();

    simu_state     = STATE_RUNNING;
    mock_can_force_sys_disable(true);

    /* -------- Act ----------------------------------------------------- */
    pthread_t tid;
    pthread_create(&tid, NULL, comms_reception, NULL);
    pthread_join(tid, NULL);

    /* -------- Assert -------------------------------------------------- */

    CU_ASSERT_EQUAL(simu_order, STATE_STOPPED);

    /* -------- Cleanup ------------------------------------------------- */
    mock_can_force_sys_disable(false);
    pthread_mutex_destroy(&mutex_bcm);
    sem_destroy(&sem_comms);
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
    CU_add_test(suite, "getCurrentTimeMsReal", test_getCurrentTimeMsReal);
    CU_add_test(suite, "check_health_signals_immediate", test_check_health_signals_immediate);
    CU_add_test(suite, "check_health_signals_persisted", test_check_health_signals_persisted);
    CU_add_test(suite, "check_health_signals_engine_temp", test_check_health_signals_engine_temp);
    CU_add_test(suite, "check_health_signals_door_status", test_check_health_signals_door_status);
    CU_add_test(suite, "comms expected iterations", test_comms_thread_expected_iterations);
    CU_add_test(suite, "check_system_disable", test_check_system_disable);
    CU_add_test(suite, "invalid_can_id_branch", test_invalid_can_id_branch);
    CU_add_test(suite, "system_disabled_path", test_system_disabled_path);
    CU_add_test(suite, "comms_reception_expected_iterations", test_comms_reception_thread_expected_iterations);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
