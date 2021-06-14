// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <btstack.h>
#include <btstack_chipset_cc256x.h>
#include <contiki-lib.h>
#include <contiki.h>
#include <tinytest_macros.h>
#include <tinytest.h>

#include <test-pbio.h>

#include "../../drv/bluetooth/bluetooth_btstack_run_loop_contiki.h"
#include "../../drv/bluetooth/bluetooth_btstack.h"
#include "../../drv/core.h"

// UART HCI queue

typedef struct {
    list_t queue;
    const uint8_t *buffer;
    uint16_t length;
} queue_item_t;

LIST(receive_queue);
PROCESS(test_uart_receive_process, "UART receive");
PROCESS(test_uart_send_process, "UART send");

static queue_item_t *new_item(const void *buffer, uint16_t length) {
    queue_item_t *item = malloc(sizeof(queue_item_t));
    assert(item);

    uint8_t *copy = malloc(length);
    assert(copy);

    memcpy(copy, buffer, length);
    item->buffer = copy;
    item->length = length;

    return item;
}

static void free_item(queue_item_t *item) {
    free((void *)item->buffer);
    free(item);
}

static void queue_packet(const uint8_t *buffer, uint16_t length) {
    list_add(receive_queue, new_item(buffer, length));
    process_poll(&test_uart_receive_process);
}

#define queue_command_complete(opcode, ...) {                       \
        static const uint8_t result[] = {                           \
            __VA_ARGS__                                             \
        };                                                          \
        _queue_command_complete(opcode, result, sizeof(result));    \
}

static void _queue_command_complete(uint16_t opcode, const uint8_t *result, uint16_t length) {
    uint8_t buffer[HCI_ACL_PAYLOAD_SIZE];

    assert(length + 6 <= HCI_ACL_PAYLOAD_SIZE);

    buffer[0] = 0x04; // packet type = Event
    buffer[1] = 0x0e; // command complete
    buffer[2] = length + 3;
    buffer[3] = 1; // number of packets
    little_endian_store_16(buffer, 4, opcode);
    memcpy(&buffer[6], result, length);

    queue_packet(buffer, length + 6);
}

// public API shared with other tests

static bool advertising_enabled;

bool pbio_test_bluetooth_is_advertising_enabled(void) {
    return advertising_enabled;
}

bool pbio_test_bluetooth_is_connected(void) {
    return hci_connection_for_handle(0x0400) != NULL;
}

void pbio_test_bluetooth_connect(void) {
    // TODO: this should probably be doing more like enumerating service, etc.
    // In other words it should trigger all of the commands that Windows/Linux/
    // Mac do when they scan and connect.

    // For now, it just sends the required command to make btstack enter the
    // connected state.

    const int length = 19;
    uint8_t buffer[length + 3];

    buffer[0] = 0x04; // packet type = Event
    buffer[1] = 0x3e; // LE Meta event
    buffer[2] = length;
    buffer[3] = 0x01; // LE Connection Complete event
    buffer[4] = 0x00; // status = successful
    little_endian_store_16(buffer, 5, 0x0400); // connection handle
    buffer[7] = 0x01; // role = slave
    buffer[8] = 0x00; // peer address type = public
    for (int i = 9; i < 15; i++) {
        buffer[i] = 0x11; // peer address = 11:11:11:11:11:11
    }
    little_endian_store_16(buffer, 15, 0x0028); // connection interval
    little_endian_store_16(buffer, 17, 0x0000); // connection latency
    little_endian_store_16(buffer, 19, 0x002a); // supervision timeout
    buffer[21] = 0x00; // master clock accuracy

    queue_packet(buffer, length + 3);
}

static void pbio_test_bluetooth_enable_notifications(uint16_t attribute_handle) {
    const uint16_t length = 5;
    uint8_t buffer[length + 9];

    buffer[0] = 0x02; // packet type = ACL Data
    little_endian_store_16(buffer, 1, 0x0400); // connection handle
    buffer[2] |= 0x02 << 4; // PB flag
    little_endian_store_16(buffer, 3, length + 4); // total data length
    little_endian_store_16(buffer, 5, length); // L2CAP length
    little_endian_store_16(buffer, 7, 4); // Attribute protocol
    buffer[9] = ATT_WRITE_REQUEST;
    little_endian_store_16(buffer, 10, attribute_handle);
    little_endian_store_16(buffer, 12, 0x0001); // value

    queue_packet(buffer, length + 9);
}

/**
 * This simulates a remote device requesting to enable notifications on the Nordic
 * UART service Tx characteristic.
 */
void pbio_test_bluetooth_enable_uart_service_notifications(void) {
    // client characteristic configuration descriptor (comes from header file generated by .gatt)
    pbio_test_bluetooth_enable_notifications(0x0014);
}

static uint32_t uart_service_notification_count;

/**
 * This count increases each time the hub sends a notification on the Nordic UART
 * service Tx characteristic.
 */
uint32_t pbio_test_bluetooth_get_uart_service_notification_count(void) {
    return uart_service_notification_count;
}

void pbio_test_bluetooth_send_uart_data(const uint8_t *data, uint32_t size) {
    // Nordic UART Rx characteristic value (comes from header file generated by .gatt)
    const uint16_t attribute_handle = 0x0011;

    uint16_t length = 3 + size;
    uint8_t buffer[9 + HCI_ACL_PAYLOAD_SIZE];
    assert(size <= HCI_ACL_PAYLOAD_SIZE - 3);

    buffer[0] = 0x02; // packet type = ACL Data
    little_endian_store_16(buffer, 1, 0x0400); // connection handle
    buffer[2] |= 0x02 << 4; // PB flag
    little_endian_store_16(buffer, 3, length + 4); // total data length
    little_endian_store_16(buffer, 5, length); // L2CAP length
    little_endian_store_16(buffer, 7, 4); // Attribute protocol
    buffer[9] = ATT_WRITE_COMMAND;
    little_endian_store_16(buffer, 10, attribute_handle);
    memcpy(&buffer[12], data, size); // value

    queue_packet(buffer, length + 9);
}

/**
 * This simulates a remote device requesting to enable notifications on the
 * Pybricks service command characteristic.
 */
void pbio_test_bluetooth_enable_pybricks_service_notifications(void) {
    // client characteristic configuration descriptor (comes from header file generated by .gatt)
    pbio_test_bluetooth_enable_notifications(0x000e);
}

static uint32_t pybricks_service_notification_count;

/**
 * This count increases each time the hub sends a notification on the Pybricks
 * service command characteristic.
 */
uint32_t pbio_test_bluetooth_get_pybricks_service_notification_count(void) {
    return pybricks_service_notification_count;
}

static pbio_test_bluetooth_control_state_t control_state;

pbio_test_bluetooth_control_state_t pbio_test_bluetooth_get_control_state(void) {
    return control_state;
}

// HCI send/receive processes

static uint8_t *test_uartreceive_buffer;
static uint16_t test_uartreceive_buffer_length;
static const uint8_t *test_uartsend_buffer;
static uint16_t test_uartsend_buffer_length;

static void (*test_uart_received_block_handler)(void);
static void (*test_uart_sent_block_handler)(void);

PROCESS_THREAD(test_uart_receive_process, ev, data) {
    static queue_item_t *item;
    static int i;

    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_UNTIL(*receive_queue);
        item = list_pop(receive_queue);

        for (i = 0; i < item->length;) {
            PROCESS_WAIT_UNTIL(test_uartreceive_buffer_length > 0);
            tt_want_uint_op(test_uartreceive_buffer_length, <=, item->length - i);
            memcpy(test_uartreceive_buffer, &item->buffer[i],
                test_uartreceive_buffer_length);

            i += test_uartreceive_buffer_length;

            test_uartreceive_buffer = NULL;
            test_uartreceive_buffer_length = 0;
            test_uart_received_block_handler();
        }

        free_item(item);
    }

    PROCESS_END();
}

// this simulates the replies from the Bluetooth chip
static void handle_send(const uint8_t *buffer, uint16_t length) {
    // this is based on simulator.py from btstack
    switch (buffer[0]) {
        case 0x01: { // Command
            uint16_t opcode = little_endian_read_16(buffer, 1);
            switch (opcode) {
                case 0x0c03: // HCI_RESET
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x1001: // HCI_READ_LOCAL_VERSION_INFO
                    queue_command_complete(opcode, 0x00, 0x10, 0x00, 0x06, 0x86, 0x1d, 0x06, 0x0a, 0x00, 0x86, 0x1d);
                    break;
                case 0x0c14: // read local name
                    queue_command_complete(opcode, 0x00, 't', 'e', 's', 't', 0x00);
                    break;
                case 0x1002: // HCI_READ_LOCAL_SUPPORTED_COMMANDS
                    queue_command_complete(opcode, 0x00, 0xff, 0xff, 0xff, 0x03, 0xfe, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xf3, 0x0f, 0xe8, 0xfe,
                        0x3f, 0xf7, 0x83, 0xff, 0x1c, 0x00, 0x00, 0x00,
                        0x61, 0xf7, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                    break;
                case 0x1009: // HCI_READ_BDADDR
                    queue_command_complete(opcode, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                    break;
                case 0x1005: // read buffer size
                    queue_command_complete(opcode, 0x00, 0x36, 0x01, 0x40, 0x0a, 0x00, 0x08, 0x00);
                    break;
                case 0x1003: // read local supported features
                    queue_command_complete(opcode, 0x00, 0xff, 0xff, 0x8f, 0xfe, 0xf8, 0xff, 0x5b, 0x87);
                    break;
                case 0x0c01: // HCI_SET_EVENT_MASK
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x2002: // LE Read Buffer Size
                    queue_command_complete(opcode, 0x00, 0x00, 0x00, 0x00);
                    break;
                case 0x0c6d: // write le host supported
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x2001: // LE Set Event Mask
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x2017: // LE Encrypt - key 16, data 16
                    queue_command_complete(opcode, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                    break;
                case 0x2006: // LE Set Advertising Parameters
                    log_debug("advertising parameters, min %d, max %d, type 0x%02x, own addr type 0x%02x, peer addr type 0x%02x, peer addr %02x:%02x:%02x:%02x:%02x:%02x, chan map 0x%02x",
                        little_endian_read_16(buffer, 4), little_endian_read_16(buffer, 6), buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15], buffer[16], buffer[17]);
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x2008: // LE Set Advertising Data
                    log_debug("advertising data, len %d", buffer[4]);
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x2009: // LE Set Scan Response Data
                    log_debug("scan response data, len %d", buffer[4]);
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0x200a: // LE Set Advertise Enable
                    advertising_enabled = buffer[4];
                    log_debug("advertising_enabled %d", advertising_enabled);
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xff36: // HCI_VS_Update_UART_HCI_Baudrate
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfe37: // HCI_VS_Start_VS_Lock
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xff05: // HCI_VS_Read_Write_Memory_Block
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xff83: // Goto_Address
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd09: // HCI_VS_Read_Modify_Write_Hardware_Register
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd0c: // HCI_VS_Sleep_Mode_Configurations
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd1c: // HCI_VS_Fast_Clock_Configuration_btip
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd76: // Send_HCI_VS_DRPb_Set_RF_Calibration_Info
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd82: // HCI_VS_DRPb_Set_Power_Vector
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd87: // HCI_VS_DRPb_Set_Class2_Single_Power
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd80: // HCI_VS_DRPb_Enable_RF_Calibration
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfe38: // HCI_VS_Stop_VS_Lock
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd2b: // HCI_VS_HCILL_Parameters
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfd5b: // HCI_VS_LE_Enable
                    queue_command_complete(opcode, 0x00);
                    break;
                case 0xfddd: // HCI_VS_LE_Output_Power
                    queue_command_complete(opcode, 0x00);
                    break;
                default:
                    tt_failprint_f(("unhandled opcode: 0x%04x", opcode));
                    break;
            }
        }
        break;

        case 0x02: { // ACL
            uint16_t connection_handle = little_endian_read_16(buffer, 1);
            uint16_t total_length = little_endian_read_16(buffer, 3);
            uint16_t length = little_endian_read_16(buffer, 5);
            uint16_t cid = little_endian_read_16(buffer, 7);

            (void)connection_handle;
            (void)total_length;
            (void)length;

            switch (cid) {
                case 0x0004: { // attribute protocol
                    uint8_t opcode = buffer[9];

                    switch (opcode) {
                        case 0x01: { // ATT_ERROR_RESPONSE
                            uint8_t failed_opcode = buffer[10];
                            uint16_t attr_handle = little_endian_read_16(buffer, 11);
                            uint8_t err_code = buffer[13];

                            tt_failprint_f(("got ATT_ERROR_RESPONSE, opcode: %02x, attr handle: %04x, err code: %02x",
                                failed_opcode, attr_handle, err_code));
                        }
                        break;

                        case 0x13: { // ATT_WRITE_RESPONSE
                            // REVISIT: maybe set a flag here?
                        }
                        break;

                        case 0x1b: { // ATT_HANDLE_VALUE_NOTIFICATION
                            uint16_t attr_handle = little_endian_read_16(buffer, 10);
                            const uint8_t *value = &buffer[12];
                            uint16_t size = length - 3;

                            // handle values come from genhrd/pybricks_service.h
                            switch (attr_handle) {
                                case 0x000d:
                                    pybricks_service_notification_count++;
                                    break;
                                case 0x0013:
                                    uart_service_notification_count++;
                                    break;
                            }

                            (void)value;
                            log_debug("ATT_HANDLE_VALUE_NOTIFICATION: attr_handle: %04x, size: %u", attr_handle, size);
                        }
                        break;

                        default:
                            tt_failprint_f(("unhandled attribute protocol opcode: 0x%0x", opcode));
                            break;
                    }
                }
                break;
                default:
                    tt_failprint_f(("unhandled ACL CID type: 0x%04x", cid));
                    break;
            }

        }
        break;

        default:
            tt_failprint_f(("unhandled packet type: 0x%02x", buffer[0]));
            break;
    }
}

PROCESS_THREAD(test_uart_send_process, ev, data) {
    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_UNTIL(test_uartsend_buffer_length > 0);

        handle_send(test_uartsend_buffer,
            test_uartsend_buffer_length);

        test_uartsend_buffer = NULL;
        test_uartsend_buffer_length = 0;
        test_uart_sent_block_handler();
    }

    PROCESS_END();
}

// test bluetooth btstack driver uart block implementation

static int test_uart_block_init(const btstack_uart_config_t *uart_config) {
    log_debug("%s", __func__);
    process_start(&test_uart_receive_process, NULL);
    process_start(&test_uart_send_process, NULL);
    return 0;
}

static int test_uart_block_open(void) {
    log_debug("%s", __func__);
    return 0;
}

static int test_uart_block_close(void) {
    log_debug("%s", __func__);
    return 0;
}

static void test_uart_block_set_block_received(void (*block_handler)(void)) {
    log_debug("%s", __func__);
    test_uart_received_block_handler = block_handler;
}

static void test_uart_block_set_block_sent(void (*block_handler)(void)) {
    log_debug("%s", __func__);
    test_uart_sent_block_handler = block_handler;
}

static int test_uart_block_set_baudrate(uint32_t baudrate) {
    return 0;
}

static void test_uart_block_receive_block(uint8_t *buffer, uint16_t length) {
    test_uartreceive_buffer = buffer;
    test_uartreceive_buffer_length = length;
    process_poll(&test_uart_receive_process);
}

static void test_uart_block_send_block(const uint8_t *buffer, uint16_t length) {
    test_uartsend_buffer = buffer;
    test_uartsend_buffer_length = length;
    process_poll(&test_uart_send_process);
}

static const btstack_uart_block_t *test_uart_block_instance(void) {
    static const btstack_uart_block_t uart_block = {
        .init = test_uart_block_init,
        .open = test_uart_block_open,
        .open = test_uart_block_close,
        .set_block_received = test_uart_block_set_block_received,
        .set_block_sent = test_uart_block_set_block_sent,
        .set_baudrate = test_uart_block_set_baudrate,
        .receive_block = test_uart_block_receive_block,
        .send_block = test_uart_block_send_block,
    };

    return &uart_block;
}

// test bluetooth btstack control driver implementation

static void test_control_init(const void *config) {
    log_debug("%s", __func__);
    control_state = PBIO_TEST_BLUETOOTH_STATE_OFF;
}

static int test_control_on(void) {
    log_debug("%s", __func__);
    control_state = PBIO_TEST_BLUETOOTH_STATE_ON;
    return 0;
}

static int test_control_off(void) {
    log_debug("%s", __func__);
    control_state = PBIO_TEST_BLUETOOTH_STATE_OFF;
    return 0;
}

static const btstack_control_t *test_control_instance(void) {
    static const btstack_control_t control = {
        .init = test_control_init,
        .on = test_control_on,
        .off = test_control_off,
    };

    return &control;
}

static const sm_key_t test_key;

const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data = {
    .uart_block_instance = test_uart_block_instance,
    .chipset_instance = btstack_chipset_cc256x_instance,
    .control_instance = test_control_instance,
    .er_key = test_key,
    .ir_key = test_key,
};

// local helpers for tests in this file

static void handle_timer_timeout(btstack_timer_source_t *ts) {
    uint32_t *callback_count = ts->context;
    (*callback_count)++;
}

static PT_THREAD(test_btstack_run_loop_contiki_timer(struct pt *pt)) {
    static btstack_timer_source_t timer_source, timer_source_2, timer_source_3;
    static uint32_t callback_count, callback_count_2, callback_count_3;

    PT_BEGIN(pt);

    // common btstack timer init
    btstack_run_loop_set_timer_handler(&timer_source, handle_timer_timeout);
    btstack_run_loop_set_timer_handler(&timer_source_2, handle_timer_timeout);
    btstack_run_loop_set_timer_handler(&timer_source_3, handle_timer_timeout);
    btstack_run_loop_set_timer_context(&timer_source, &callback_count);
    btstack_run_loop_set_timer_context(&timer_source_2, &callback_count_2);
    btstack_run_loop_set_timer_context(&timer_source_3, &callback_count_3);

    // -- test single timer callback --

    // init and schedule
    callback_count = 0;
    btstack_run_loop_set_timer(&timer_source, 10);
    btstack_run_loop_add_timer(&timer_source);

    // should not expire early
    clock_tick(9);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    // now it should be done
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);


    // -- timers scheduled out of order should fire in order --

    callback_count = callback_count_2 = callback_count_3 = 0;
    btstack_run_loop_set_timer(&timer_source, 10);
    btstack_run_loop_set_timer(&timer_source_2, 5);
    btstack_run_loop_set_timer(&timer_source_3, 15);
    btstack_run_loop_add_timer(&timer_source);
    btstack_run_loop_add_timer(&timer_source_2);
    btstack_run_loop_add_timer(&timer_source_3);

    // should not expire early
    clock_tick(4);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    tt_want_uint_op(callback_count_2, ==, 0);
    tt_want_uint_op(callback_count_3, ==, 0);

    // only timer 2 should be called back after 5 ms
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 0);

    // then timer 1 after 10 ms
    clock_tick(5);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 0);

    // and finally timer 3
    clock_tick(5);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 1);


    // -- timers with same timeout should all call back at the same time

    callback_count = callback_count_2 = callback_count_3 = 0;
    btstack_run_loop_set_timer(&timer_source, 15);
    btstack_run_loop_add_timer(&timer_source);

    clock_tick(5);
    PT_YIELD(pt);

    btstack_run_loop_set_timer(&timer_source_2, 10);
    btstack_run_loop_add_timer(&timer_source_2);

    clock_tick(5);
    PT_YIELD(pt);

    btstack_run_loop_set_timer(&timer_source_3, 5);
    btstack_run_loop_add_timer(&timer_source_3);

    // none should have timeout out yet
    clock_tick(4);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    tt_want_uint_op(callback_count_2, ==, 0);
    tt_want_uint_op(callback_count_3, ==, 0);

    // then all at the same time
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);
    tt_want_uint_op(callback_count_2, ==, 1);
    tt_want_uint_op(callback_count_3, ==, 1);


    // -- should be able to cancel a timer --

    // init and schedule
    callback_count = 0;
    btstack_run_loop_set_timer_handler(&timer_source, handle_timer_timeout);
    btstack_run_loop_set_timer_context(&timer_source, &callback_count);
    btstack_run_loop_set_timer(&timer_source, 10);
    btstack_run_loop_add_timer(&timer_source);

    // should not expire early
    clock_tick(9);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);
    btstack_run_loop_remove_timer(&timer_source);

    // it should have been canceled
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    PT_END(pt);
}

static void handle_data_source(btstack_data_source_t *ds,  btstack_data_source_callback_type_t callback_type) {
    uint32_t *callback_count = ds->source.handle;

    switch (callback_type) {
        case DATA_SOURCE_CALLBACK_POLL:
            (*callback_count)++;
            break;
        default:
            break;
    }
}

static PT_THREAD(test_btstack_run_loop_contiki_poll(struct pt *pt)) {
    static btstack_data_source_t data_source;
    static uint32_t callback_count;

    PT_BEGIN(pt);

    callback_count = 0;
    btstack_run_loop_set_data_source_handle(&data_source, &callback_count);
    btstack_run_loop_set_data_source_handler(&data_source, &handle_data_source);
    btstack_run_loop_enable_data_source_callbacks(&data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&data_source);

    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 1);

    callback_count = 0;
    btstack_run_loop_disable_data_source_callbacks(&data_source, DATA_SOURCE_CALLBACK_POLL);
    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    callback_count = 0;
    btstack_run_loop_enable_data_source_callbacks(&data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_remove_data_source(&data_source);
    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
    PT_YIELD(pt);
    tt_want_uint_op(callback_count, ==, 0);

    PT_END(pt);
}

struct testcase_t pbdrv_bluetooth_tests[] = {
    PBIO_PT_THREAD_TEST(test_btstack_run_loop_contiki_timer),
    PBIO_PT_THREAD_TEST(test_btstack_run_loop_contiki_poll),
    END_OF_TESTCASES
};
