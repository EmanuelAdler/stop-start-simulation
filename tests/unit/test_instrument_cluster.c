#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../../src/instrument_cluster/instrument_cluster_func.h"
#include "../../src/common_includes/logging.h"

#define FILE_LINE_SIZE (512)
#define MOCK_SOCKET    (123)

/* Suite init/cleanup (do nothing special here) */
static int init_suite(void)   { return 0; }
static int clean_suite(void)  { return 0; }

/* Utility: checks if a given file contains a particular substring */
static bool file_contains_substring(const char *filepath, const char *substring) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        return false;
    }
    char line[FILE_LINE_SIZE];
    bool found = false;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, substring)) {
            found = true;
            break;
        }
    }
    fclose(fp);
    return found;
}

/* 
 * TEST 1: Passing "press_start_stop"
 *         -> Should log "[INFO] System button toggled"
 *         -> Should NOT print "Invalid input" 
 */
void test_check_input_press_start_stop(void)
{
    /* Set a unique log file path so we don't overwrite real logs */
    const char *log_path = "/tmp/test_ic_press_start_stop.log";
    set_log_file_path(log_path);

    /* Initialize logging */
    bool ok = init_logging_system();
    CU_ASSERT_TRUE_FATAL(ok);

    /* Redirect stdout to capture console output */
    const char *stdout_file = "/tmp/stdout_capture.txt";
    FILE *saved_stdout = stdout; 
    freopen(stdout_file, "w", stdout);

    /* Call the function under test */
    check_input_command("press_start_stop", MOCK_SOCKET);

    /* Restore original stdout */
    fflush(stdout);
    stdout = saved_stdout;

    /* Cleanup logging */
    cleanup_logging_system();

    /* Check that "Invalid input" was NOT printed */
    bool hasInvalid = file_contains_substring(stdout_file, "Invalid input. Try again.");
    CU_ASSERT_FALSE(hasInvalid);

    /* Check that the log includes "[INFO] System button toggled" */
    bool hasInfoLog = file_contains_substring(log_path, "[INFO] System button toggled");
    CU_ASSERT_TRUE(hasInfoLog);
}

/* 
 * TEST 2: Passing "show_dashboard"
 *         -> Should call send_encrypted_message (no direct log_toggle_event)
 *         -> Should NOT print "Invalid input"
 *         -> Should NOT log "[INFO] System button toggled"
 */
void test_check_input_show_dashboard(void)
{
    const char *log_path = "/tmp/test_ic_show_dashboard.log";
    set_log_file_path(log_path);

    bool ok = init_logging_system();
    CU_ASSERT_TRUE_FATAL(ok);

    const char *stdout_file = "/tmp/stdout_capture.txt";
    FILE *saved_stdout = stdout;
    freopen(stdout_file, "w", stdout);

    check_input_command("show_dashboard", MOCK_SOCKET);

    fflush(stdout);
    stdout = saved_stdout;

    cleanup_logging_system();

    /* "Invalid input" should NOT appear */
    bool hasInvalid = file_contains_substring(stdout_file, "Invalid input. Try again.");
    CU_ASSERT_FALSE(hasInvalid);

    /* Should NOT log "[INFO] System button toggled" for show_dashboard */
    bool hasInfoLog = file_contains_substring(log_path, "[INFO] System button toggled");
    CU_ASSERT_FALSE(hasInfoLog);
}

/* 
 * TEST 3: Passing an invalid command
 *         -> Should print "Invalid input. Try again."
 *         -> Should NOT log anything 
 */
void test_check_input_invalid(void)
{
    const char *log_path = "/tmp/test_ic_invalid.log";
    set_log_file_path(log_path);

    bool ok = init_logging_system();
    CU_ASSERT_TRUE_FATAL(ok);

    const char *stdout_file = "/tmp/stdout_capture.txt";
    FILE *saved_stdout = stdout;
    freopen(stdout_file, "w", stdout);

    check_input_command("some_invalid_command", MOCK_SOCKET);

    fflush(stdout);
    stdout = saved_stdout;

    cleanup_logging_system();

    /* "Invalid input" SHOULD appear on stdout */
    bool hasInvalid = file_contains_substring(stdout_file, "Invalid input. Try again.");
    CU_ASSERT_TRUE(hasInvalid);

    /* Should NOT contain "[INFO] System button toggled" */
    bool hasInfoLog = file_contains_substring(log_path, "[INFO] System button toggled");
    CU_ASSERT_FALSE(hasInfoLog);
}

/* 
 * Register tests in the CUnit suite 
 */
int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("InstrumentClusterSuite", init_suite, clean_suite);
    if (!suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add test cases */
    CU_add_test(suite, "press_start_stop", test_check_input_press_start_stop);
    CU_add_test(suite, "show_dashboard",   test_check_input_show_dashboard);
    CU_add_test(suite, "invalid_command",  test_check_input_invalid);

    /* Run tests in verbose mode */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return 0;
}
