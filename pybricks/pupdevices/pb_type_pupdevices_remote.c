// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <stdint.h>

#include <lego_lwp3.h>

#include <pbdrv/bluetooth.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/task.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

typedef struct {
    pbio_task_t task;
    uint8_t left[3];
    uint8_t right[3];
    uint8_t center;
    pbdrv_bluetooth_scan_and_connect_context_t context;
} pb_remote_t;

STATIC pb_remote_t pb_remote_singleton;

// Handles LEGO Wireless protocol messages from the handset
STATIC void handle_notification(pbdrv_bluetooth_connection_t connection, const uint8_t *value, uint8_t size) {
    pb_remote_t *remote = &pb_remote_singleton;

    if (value[0] == 5 && value[2] == LWP3_MSG_TYPE_HW_NET_CMDS && value[3] == LWP3_HW_NET_CMD_CONNECTION_REQ) {
        // This message is meant for something else, but contains the center button state
        remote->center = value[4];
    } else if (value[0] == 7 && value[2] == LWP3_MSG_TYPE_PORT_VALUE) {
        // This assumes that the handset button ports have already been set to mode KEYSD
        if (value[3] == 0) {
            memcpy(remote->left, &value[4], 3);
        } else if (value[3] == 1) {
            memcpy(remote->right, &value[4], 3);
        }
    }
}

STATIC void pb_remote_connect(mp_int_t timeout) {
    pb_remote_t *remote = &pb_remote_singleton;

    memset(remote, 0, sizeof(*remote));

    // TODO: check for busy

    pbdrv_bluetooth_set_notification_handler(handle_notification);
    pbdrv_bluetooth_scan_and_connect(&remote->task, &remote->context);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_int_t start = mp_hal_ticks_ms();
        while (timeout == -1 || mp_hal_ticks_ms() - start < timeout) {
            MICROPY_EVENT_POLL_HOOK
            if (remote->task.status != PBIO_ERROR_AGAIN) {
                nlr_pop();
                pb_assert(remote->task.status);
                return;
            }
        }
        mp_raise_OSError(MP_ETIMEDOUT);
        nlr_pop();
    } else {
        pbio_task_cancel(&remote->task);
        while (remote->task.status == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }
        nlr_jump(nlr.ret_val);
    }
}

void pb_type_Remote_cleanup(void) {
    pbdrv_bluetooth_disconnect_remote();
}

STATIC pbio_error_t remote_button_is_pressed(pbio_button_flags_t *pressed) {
    pb_remote_t *remote = &pb_remote_singleton;

    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_HANDSET)) {
        mp_raise_OSError(MP_ENODEV);
    }

    *pressed = 0;

    if (remote->left[0]) {
        *pressed |= PBIO_BUTTON_LEFT_UP;
    }
    if (remote->left[1]) {
        *pressed |= PBIO_BUTTON_LEFT;
    }
    if (remote->left[2]) {
        *pressed |= PBIO_BUTTON_LEFT_DOWN;
    }
    if (remote->right[0]) {
        *pressed |= PBIO_BUTTON_RIGHT_UP;
    }
    if (remote->right[1]) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }
    if (remote->right[2]) {
        *pressed |= PBIO_BUTTON_RIGHT_DOWN;
    }
    if (remote->center) {
        *pressed |= PBIO_BUTTON_CENTER;
    }
    return PBIO_SUCCESS;
}

typedef struct _pb_type_pupdevices_Remote_obj_t {
    mp_obj_base_t base;
    mp_obj_t buttons;
} pb_type_pupdevices_Remote_obj_t;

STATIC const pb_obj_enum_member_t *remote_buttons[] = {
    &pb_Button_LEFT_MINUS_obj,
    &pb_Button_RIGHT_MINUS_obj,
    &pb_Button_LEFT_obj,
    &pb_Button_CENTER_obj,
    &pb_Button_RIGHT_obj,
    &pb_Button_LEFT_PLUS_obj,
    &pb_Button_RIGHT_PLUS_obj
};

STATIC mp_obj_t pb_type_pupdevices_Remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_pupdevices_Remote_obj_t *self = m_new_obj(pb_type_pupdevices_Remote_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    if (name_in != mp_const_none) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("filter by name is not implemented"));
    }

    mp_int_t timeout = timeout_in == mp_const_none? -1 : pb_obj_get_positive_int(timeout_in);
    pb_remote_connect(timeout);

    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(remote_buttons), remote_buttons, remote_button_is_pressed);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t pb_type_pupdevices_Remote_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_ATTRIBUTE_OFFSET(pb_type_pupdevices_Remote_obj_t, buttons) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_pupdevices_Remote_locals_dict, pb_type_pupdevices_Remote_locals_dict_table);

const mp_obj_type_t pb_type_pupdevices_Remote = {
    { &mp_type_type },
    .name = PYBRICKS_HUB_CLASS_NAME,
    .make_new = pb_type_pupdevices_Remote_make_new,
    .locals_dict = (mp_obj_dict_t *)&pb_type_pupdevices_Remote_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
