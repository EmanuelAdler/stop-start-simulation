#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <linux/can.h>
#include <sys/stat.h>
#include "../../src/common_includes/can_socket.h"

/* We'll define a test interface & some constants */
#define TEST_INTERFACE      "vcan0"
#define NONEXIST_INTERFACE  "vcan99"   // valid name length, but likely doesn't exist
#define EMPTY_INTERFACE     ""         // triggers "Invalid interface name"
#define LONG_INTERFACE      "abcdefghijklmnop" // 16 chars => invalid
#define BUFFER_SIZE         (128)
#define CAN_ID_FAKE         (0x321)

/* For frames */
#define TEST_CAN_ID         0x123
#define TEST_CAN_DLC        2
#define TEST_DATA_0         0xAB
#define TEST_DATA_1         0xCD

/* A small utility to see if vcan0 is likely up. */
static bool is_vcan_available(void)
{
    struct stat sbuff;
    return (stat("/sys/class/net/vcan0", &sbuff) == 0);
}

/* Suite init/cleanup (no special steps here) */
static int init_suite(void) { return 0; }
static int clean_suite(void) { return 0; }

/* -----------------------------------------------------------------------------
 * Test: create_can_socket with a valid interface
 * ---------------------------------------------------------------------------*/
static void test_create_can_socket(void)
{
    if (!is_vcan_available()) 
    {
        CU_FAIL("vcan0 not available. Skipping real socket test.");
        return;
    }
    int sock = create_can_socket(TEST_INTERFACE);
    CU_ASSERT_TRUE(sock >= 0);
    if (sock >= 0) 
    {
        close_can_socket(sock);
    }
}

/* -----------------------------------------------------------------------------
 * Test: create_can_socket with an empty interface => triggers invalid name
 * ---------------------------------------------------------------------------*/
static void test_create_can_socket_empty(void)
{
    int sock = create_can_socket(EMPTY_INTERFACE);
    CU_ASSERT_EQUAL(sock, -1);
}

/* -----------------------------------------------------------------------------
 * Test: create_can_socket with a 16-char name => also invalid
 * ---------------------------------------------------------------------------*/
static void test_create_can_socket_too_long(void)
{
    int sock = create_can_socket(LONG_INTERFACE); 
    CU_ASSERT_EQUAL(sock, -1);
}

/* -----------------------------------------------------------------------------
 * Test: create_can_socket with a *valid-length* name that doesn't exist
 *        => likely fails on ioctl(..., SIOCGIFINDEX, ...)
 * ---------------------------------------------------------------------------*/
static void test_create_can_socket_nonexistent(void)
{
    int sock = create_can_socket(NONEXIST_INTERFACE);
    CU_ASSERT_EQUAL(sock, -1);
}

/* -----------------------------------------------------------------------------
 * Test: send_can_frame() with a valid socket
 *        We'll create a socket, then attempt to send a short CAN frame.
 *        If vcan0 is up, it *should* succeed, returning 0 => OPERATION_SUCCESS
 * ---------------------------------------------------------------------------*/
static void test_send_can_frame_valid(void)
{
    if (!is_vcan_available()) {
        CU_FAIL("vcan0 not available. Skipping real socket test.");
        return;
    }

    int sock = create_can_socket(TEST_INTERFACE);
    CU_ASSERT_TRUE(sock >= 0);
    if (sock < 0) 
    {
        return;
    }

    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.can_id  = TEST_CAN_ID;
    frame.can_dlc = TEST_CAN_DLC;
    frame.data[0] = TEST_DATA_0;
    frame.data[1] = TEST_DATA_1;

    int ret = send_can_frame(sock, &frame);
    /* If everything is okay, ret should be 0 => OPERATION_SUCCESS */
    CU_ASSERT_EQUAL(ret, 0);

    close_can_socket(sock);
}

/* -----------------------------------------------------------------------------
 * Test: send_can_frame() with invalid socket
 * ---------------------------------------------------------------------------*/
static void test_send_can_frame_invalid_socket(void)
{
    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.can_id  = TEST_CAN_ID;
    frame.can_dlc = TEST_CAN_DLC;
    frame.data[0] = TEST_DATA_0;
    frame.data[1] = TEST_DATA_1;

    int ret = send_can_frame(-1, &frame);
    CU_ASSERT_EQUAL(ret, -1);
}

/* -----------------------------------------------------------------------------
 * Test: receive_can_frame() with invalid socket
 * ---------------------------------------------------------------------------*/
static void test_receive_can_frame_invalid_socket(void)
{
    struct can_frame frame;
    int ret = receive_can_frame(-1, &frame);
    CU_ASSERT_EQUAL(ret, -1);
}

/* -----------------------------------------------------------------------------
 * Test: close_can_socket with valid socket
 * ---------------------------------------------------------------------------*/
static void test_close_can_socket(void)
{
    if (!is_vcan_available()) {
        CU_FAIL("vcan0 not available. Skipping real socket test.");
        return;
    }
    int sock = create_can_socket(TEST_INTERFACE);
    CU_ASSERT_TRUE(sock >= 0);

    close_can_socket(sock);
}

/* -----------------------------------------------------------------------------
 * Test: encrypt_data() + decrypt_data()
 *        Ensures we cover those code paths.
 * ---------------------------------------------------------------------------*/
static void test_encrypt_decrypt(void)
{
    unsigned char plain[AES_BLOCK_SIZE];
    unsigned char cipher[AES_BLOCK_SIZE];
    char recovered[AES_BLOCK_SIZE + 1];
    int out_len = 0;

    memset(plain, 'A', AES_BLOCK_SIZE); // 16 'A'
    memset(cipher, 0, AES_BLOCK_SIZE);
    memset(recovered, 0, AES_BLOCK_SIZE + 1);

    encrypt_data(plain, cipher, &out_len);
    /* Expect out_len == 16 (AES_BLOCK_SIZE) */
    CU_ASSERT_EQUAL(out_len, AES_BLOCK_SIZE);

    decrypt_data(cipher, recovered, out_len);
    /* Now recovered should be "AAAAAAAAAAAAAAAA" (16 'A') */
    CU_ASSERT_STRING_EQUAL(recovered, "AAAAAAAAAAAAAAAA");
}

/* -----------------------------------------------------------------------------
 * Test: send_encrypted_message()
 *        Check if it attempts to send 2 frames
 *        (the second half of the 16 bytes).
 * ---------------------------------------------------------------------------*/
/**
 * @test test_send_encrypted_message
 * @brief Test if an encrypted message is sent via CAN socket
 * @req SWR1.4
 * @file unit/test_can_socket.c
 */
static void test_send_encrypted_message(void)
{
    if (!is_vcan_available()) {
        CU_FAIL("vcan0 not available. Skipping real socket test.");
        return;
    }
    int sock = create_can_socket(TEST_INTERFACE);
    CU_ASSERT_TRUE(sock >= 0);
    if (sock < 0) 
    {
        return;
    }

    /* "HelloWorld" is less than 16 chars. The code pads it (strncpy). */
    send_encrypted_message(sock, "HelloWorld", CAN_ID_FAKE);

    close_can_socket(sock);
}

int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    CU_pSuite suite = CU_add_suite("CAN Socket Test Suite", init_suite, clean_suite);
    if (!suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add tests */
    CU_add_test(suite, "create_can_socket valid",           test_create_can_socket);
    CU_add_test(suite, "create_can_socket empty",           test_create_can_socket_empty);
    CU_add_test(suite, "create_can_socket too long",        test_create_can_socket_too_long);
    CU_add_test(suite, "create_can_socket nonexistent",     test_create_can_socket_nonexistent);
    CU_add_test(suite, "send_can_frame valid",              test_send_can_frame_valid);
    CU_add_test(suite, "send_can_frame invalid socket",     test_send_can_frame_invalid_socket);
    CU_add_test(suite, "receive_can_frame invalid socket",  test_receive_can_frame_invalid_socket);
    CU_add_test(suite, "close_can_socket valid",            test_close_can_socket);
    CU_add_test(suite, "encrypt/decrypt",                   test_encrypt_decrypt);
    CU_add_test(suite, "send_encrypted_message",            test_send_encrypted_message);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int fails = CU_get_number_of_failures();
    CU_cleanup_registry();
    return (fails == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
