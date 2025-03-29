#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "../../src/common_includes/logging.h"

#define NUM_THREADS (5)
#define FILE_LINE_SIZE (512)
#define BUFFER_SIZE (64)

//-------------------------------------
// Setup the CUnit Suite
//-------------------------------------
static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

//-------------------------------------
// Test 1: Successful Initialization
//-------------------------------------
void test_logging_init_success(void)
{
    // Use a temp path so we don't overwrite real logs
    const char *log_path = "/tmp/test_log_success.log";
    set_log_file_path(log_path);

    // Initialize
    bool result = init_logging_system();
    CU_ASSERT_TRUE(result);

    // Verify the file now exists (access() == 0 means success)
    CU_ASSERT_EQUAL(access(log_path, F_OK), 0);

    // Cleanup
    cleanup_logging_system();
}

//-------------------------------------
// Test 2: Failure Initialization
//-------------------------------------
void test_logging_init_failure(void)
{
    // Use a path we expect to fail to open (e.g., /root/... for normal user)
    const char *log_path = "/root/test_log_fail.log";
    set_log_file_path(log_path);

    bool result = init_logging_system();
    CU_ASSERT_FALSE(result);

    // File should not exist (or at least be inaccessible)
    // We expect access() != 0 (but ignore the exact errno)
    CU_ASSERT_NOT_EQUAL(access(log_path, F_OK), 0);

    // Cleanup
    cleanup_logging_system();
}

//-------------------------------------
// Utility to find a given substring in the file
//-------------------------------------

typedef struct
{
    const char *filepath;
    const char *substring;
} f_susbtring_data;

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
// Test 3: Concurrency
//-------------------------------------
static void *thread_logging_fn(void *arg)
{
    // Each thread logs a distinct message based on its ID
    long tid = (long)arg;
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Thread %ld logging message", tid);
    log_toggle_event(buffer);

    return NULL;
}

void test_logging_concurrency(void)
{
    const char *log_path = "/tmp/test_log_threads.log";
    set_log_file_path(log_path);

    bool result = init_logging_system();
    CU_ASSERT_TRUE_FATAL(result);

    // Spawn multiple threads, each logs a unique message
    pthread_t threads[NUM_THREADS];
    for (long i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, thread_logging_fn, (void *)i);
    }
    // Join all threads
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Cleanup (forces all data to be flushed/closed)
    cleanup_logging_system();

    // Verify each thread’s message is in the file
    for (int i = 0; i < NUM_THREADS; i++)
    {
        char expected[BUFFER_SIZE];
        snprintf(expected, sizeof(expected), "Thread %d logging message", i);

        f_susbtring_data params_str;

        params_str.filepath = log_path;
        params_str.substring = expected;

        CU_ASSERT_TRUE(file_contains_substring(params_str));
    }
}

//-------------------------------------
// Test 4: After Cleanup
//-------------------------------------
void test_logging_after_cleanup(void)
{
    const char *log_path = "/tmp/test_log_after_cleanup.log";
    set_log_file_path(log_path);

    // Initialize & write once
    bool result = init_logging_system();
    CU_ASSERT_TRUE_FATAL(result);
    const char *first_message = "Test: normal usage";
    log_toggle_event((char *)first_message);

    // Cleanup
    cleanup_logging_system();

    // Attempt to log again (should be ignored / no-op)
    const char *second_message = "Test: after cleanup";
    log_toggle_event((char *)second_message);

    f_susbtring_data params_str;

    params_str.filepath = log_path;
    params_str.substring = first_message;

    // Now verify the file
    // Only the first message should be present
    CU_ASSERT_TRUE(file_contains_substring(params_str));

    params_str.substring = second_message;

    CU_ASSERT_FALSE(file_contains_substring(params_str));
}

int main(void)
{
    // Initialize CUnit test registry
    if (CU_initialize_registry() != CUE_SUCCESS)
    {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("LoggingSuite", init_suite, clean_suite);
    if (!suite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests
    CU_add_test(suite, "test init success", test_logging_init_success);
    CU_add_test(suite, "test init failure", test_logging_init_failure);
    CU_add_test(suite, "test concurrency", test_logging_concurrency);
    CU_add_test(suite, "test after cleanup", test_logging_after_cleanup);

    // Run all tests in verbose mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();
    CU_cleanup_registry();

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
