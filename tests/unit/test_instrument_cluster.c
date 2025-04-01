#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../src/instrument_cluster/instrument_cluster_func.h"
#include "../../src/common_includes/logging.h"

#define MOCK_SOCKET    (999)

//-------------------------------------
// Declare the extra "mock" functions created
//-------------------------------------
int stub_can_get_send_count(void);
const char* stub_can_get_last_message(void);
void stub_can_reset(void);

//-------------------------------------
// Setup the CUnit Suite
//-------------------------------------
static int init_suite(void)  { stub_can_reset(); return 0; }
static int clean_suite(void) { return 0; }

//-------------------------------------
// Test 1: Use the command "press_start_stop"
//-------------------------------------
void test_press_start_stop(void)
{
    // Initialize logging just so we don't get an error
    set_log_file_path("/tmp/test_ic_stub.log");
    bool is_log_ok = init_logging_system();
    CU_ASSERT_TRUE_FATAL(is_log_ok);

    // Reset the stub counter
    stub_can_reset();

    // Call the function
    check_input_command("press_start_stop", MOCK_SOCKET);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);
    // Check if the final message matches the expected one
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "press_start_stop");

    // Cleanup
    cleanup_logging_system();
}

//-------------------------------------
// Test 2: Use the command "show_dashboard"
//-------------------------------------
void test_show_dashboard(void)
{
    // Initialize logging just so we don't get an error
    set_log_file_path("/tmp/test_ic_stub.log");
    bool is_log_ok = init_logging_system();
    CU_ASSERT_TRUE_FATAL(is_log_ok);

    // Reset the stub counter
    stub_can_reset();

    // Call the function
    check_input_command("show_dashboard", MOCK_SOCKET);

    // Check if the stub was called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 1);
    // Check if the final message matches the expected one
    CU_ASSERT_STRING_EQUAL(stub_can_get_last_message(), "show_dashboard");

    // Cleanup
    cleanup_logging_system();
}

//-------------------------------------
// Test 3: Try to use an invalid command
//-------------------------------------
void test_invalid_command(void)
{
    stub_can_reset();

    check_input_command("foobar", MOCK_SOCKET);
    // We expect that send_encrypted_message was not called
    CU_ASSERT_EQUAL(stub_can_get_send_count(), 0);
}

int main(void)
{
    // Initialize CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("InstrumentClusterSuite", init_suite, clean_suite);
    if (!suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests
    CU_add_test(suite, "press_start_stop", test_press_start_stop);
    CU_add_test(suite, "show_dashboard", test_show_dashboard);
    CU_add_test(suite, "invalid_command",  test_invalid_command);

    // Run all tests in verbose mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
