// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_TYPE_DEVICE_H
#define PYBRICKS_INCLUDED_PYBRICKS_TYPE_DEVICE_H

#include "py/mpconfig.h"

#include "py/obj.h"

#include <pbdrv/legodev.h>

#include <pbdrv/legodev.h>
#include <pybricks/tools/pb_type_awaitable.h>

/**
 * Used in place of mp_obj_base_t in all pupdevices. This lets us share
 * code for awaitables for all pupdevices.
 */
typedef struct _pb_type_device_obj_base_t {
    mp_obj_base_t base;
    pbdrv_legodev_dev_t *legodev;
    mp_obj_t awaitables;
} pb_type_device_obj_base_t;

#if PYBRICKS_PY_DEVICES

/**
 * Callable sensor method with a particular mode and return map.
 */
typedef struct {
    mp_obj_base_t base;
    pb_type_awaitable_return_t get_values;
    uint8_t mode;
} pb_type_device_method_obj_t;

extern const mp_obj_type_t pb_type_device_method;

#define PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(obj_name, mode_id, get_values_func) \
    const pb_type_device_method_obj_t obj_name = \
    {{&pb_type_device_method}, .mode = mode_id, .get_values = get_values_func}

mp_obj_t pb_type_device_method_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t pb_type_pupdevices_method(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
pbdrv_legodev_type_id_t pb_type_device_init_class(pb_type_device_obj_base_t *self, mp_obj_t port_in, pbdrv_legodev_type_id_t valid_id);
mp_obj_t pb_type_device_set_data(pb_type_device_obj_base_t *sensor, uint8_t mode, const void *data, uint8_t size);
void *pb_type_device_get_data(mp_obj_t self_in, uint8_t mode);

void *pb_type_device_get_data_blocking(mp_obj_t self_in, uint8_t mode);

#endif // PYBRICKS_PY_DEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_TYPE_DEVICE_H
