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

//-------------------------------------
// Setup the CUnit Suite
//-------------------------------------
static int init_suite(void)  { return 0; }
static int clean_suite(void) { return 0; }

/* Utility to search for a substring in a file */
static bool file_contains_substring(const char *path, const char *substr)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        return false;
    }

    char line[FILE_LINE_SIZE];
    bool found = false;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, substr)) {
            found = true;
            break;
        }
    }
    fclose(fp);
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
    CU_ASSERT_TRUE(file_contains_substring("/tmp/test_dashboard_frame.log", 
                                           "[INFO] System Activated"));
}

//-------------------------------------
// Test 2: Test the output for print dashboard status
//-------------------------------------
void test_print_dashboard_status(void)
{
    // 1) Set the actuator state as we like
    actuators.start_stop_active = false;

    // 2) Saves the state of the “real” stdout
    int saved_stdout_fd = dup(STDOUT_FILENO);

    // 3) Open temporary file
    const char *stdout_file = "/tmp/test_dashboard_stdout.txt";
    FILE *fp = fopen(stdout_file, "w");
    CU_ASSERT_PTR_NOT_NULL_FATAL(fp);

    // 4) Redirect stdout to this file
    int temp_fd = fileno(fp);
    dup2(temp_fd, STDOUT_FILENO);

    // 5) Call the function
    print_dashboard_status();

    // 6) Restore original stdout
    fflush(stdout);
    dup2(saved_stdout_fd, STDOUT_FILENO);
    close(saved_stdout_fd);
    fclose(fp);

    // 7) Now check the file for expected text
    CU_ASSERT_TRUE(file_contains_substring(stdout_file, "=== Dashboard Status ==="));
    CU_ASSERT_TRUE(file_contains_substring(stdout_file, "Stop/Start button: 0"));
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

    // Run all tests in verbose mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
