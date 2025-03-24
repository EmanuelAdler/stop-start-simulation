#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "../../src/dashboard/dashboard_func.h" // or .c if you have no .h
#include <stdbool.h>

// A minimal example
static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

void test_dashboard_dummy(void)
{
    // some test calls or asserts
    CU_ASSERT_TRUE(true);
}

int main(void)
{
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("DashboardSuite", init_suite, clean_suite);
    CU_add_test(suite, "test dashboard dummy", test_dashboard_dummy);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return 0;
}
