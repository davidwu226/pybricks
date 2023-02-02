// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <pbio/angle.h>
#include <pbio/dcmotor.h>
#include <pbio/int_math.h>
#include <pbio/observer.h>
#include <pbio/trajectory.h>

// Values generated by pbio/doc/control/model.py
#define MAX_NUM_SPEED (2500000)
#define MAX_NUM_ACCELERATION (2500000)
#define MAX_NUM_CURRENT (30000)
#define MAX_NUM_VOLTAGE (12000)
#define MAX_NUM_TORQUE (1000000)
#define PRESCALE_SPEED (858)
#define PRESCALE_ACCELERATION (858)
#define PRESCALE_CURRENT (71582)
#define PRESCALE_VOLTAGE (178956)
#define PRESCALE_TORQUE (2147)

/**
 * Resets the observer to a new angle. Speed and current are reset to zero.
 *
 * @param [in]  obs            The observer instance.
 * @param [in]  angle          Angle to which the observer should be reset.
 */
void pbio_observer_reset(pbio_observer_t *obs, const pbio_angle_t *angle) {

    // Reset angle to input and other states to zero.
    obs->angle = *angle;
    obs->speed = 0;
    obs->current = 0;

    // Reset stall state.
    obs->stalled = false;

    // Reset position differentiator.
    pbio_differentiator_reset(&obs->differentiator, angle);
}

/**
 * Gets the observer state, which is the estimated state of the real system.
 *
 * @param [in]  obs            The observer instance.
 * @param [out] speed_num      Speed in millidegrees/second as numeric derivative of angle.
 * @param [out] angle_est      Model estimate of angle in millidegrees.
 * @param [out] speed_est      Model estimate of speed in millidegrees/second.
 */
void pbio_observer_get_estimated_state(const pbio_observer_t *obs, int32_t *speed_num, pbio_angle_t *angle_est, int32_t *speed_est) {
    // Return angle in millidegrees.
    *angle_est = obs->angle;

    // Return speed in millidegrees per second.
    *speed_est = obs->speed;
    *speed_num = obs->speed_numeric;
}

static void update_stall_state(pbio_observer_t *obs, uint32_t time, pbio_dcmotor_actuation_t actuation, int32_t voltage, int32_t feedback_voltage) {

    // Anything other than voltage actuation is not included in the observer
    // model, so it should not cause any stall flags to be raised.
    if (actuation != PBIO_DCMOTOR_ACTUATION_VOLTAGE) {
        obs->stalled = false;
        return;
    }

    // Convert to forward motion to simplify checks.
    int32_t speed = obs->speed;
    if (voltage < 0) {
        speed *= -1;
        voltage *= -1;
        feedback_voltage *= -1;
    }

    // Check stall conditions.
    if (// Motor is going slow or even backward.
        speed < obs->settings.stall_speed_limit &&
        // Model is ahead of reality (and therefore pushing back negative),
        // indicating an unmodelled load.
        feedback_voltage < 0 &&
        // Feedback voltage is more 75% of what it would be on getting
        // fully stuck (where applied voltage equals feedback).
        -feedback_voltage > (voltage * 3) / 4 &&
        // Feedback voltage is nonnegligible, i.e. larger than friction torque.
        voltage > 5 * pbio_observer_torque_to_voltage(obs->model, obs->model->torque_friction / 2)
        ) {
        // If this is the rising edge of the stall flag, reset start time.
        if (!obs->stalled) {
            obs->stall_start = time;
        }
        obs->stalled = true;
    } else {
        // Otherwise the motor is not stalled.
        obs->stalled = false;
    }
}

/**
 * Gets observer feedback voltage that keep it close to measured value.
 *
 * @param [in]  obs            The observer instance.
 * @param [in]  angle          Measured angle used to correct the model.
 * @return                     Feedback voltage in mV.
 */
int32_t pbio_observer_get_feedback_voltage(pbio_observer_t *obs, const pbio_angle_t *angle) {
    int32_t error = pbio_angle_diff_mdeg(angle, &obs->angle);
    return pbio_int_math_clamp(pbio_control_settings_mul_by_gain(error, obs->settings.feedback_gain), MAX_NUM_VOLTAGE);
}

/**
 * Predicts next system state and corrects the model using a measurement.
 *
 * @param [in]  obs            The observer instance.
 * @param [in]  time           Wall time.
 * @param [in]  angle          Measured angle used to correct the model.
 * @param [in]  actuation      Actuation type currently applied to the motor.
 * @param [in]  voltage        If actuation type is voltage, this is the payload in mV.
 */
void pbio_observer_update(pbio_observer_t *obs, uint32_t time, const pbio_angle_t *angle, pbio_dcmotor_actuation_t actuation, int32_t voltage) {

    const pbio_observer_model_t *m = obs->model;

    if (actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        // TODO
    }

    // Update numerical derivative as speed sanity check.
    obs->speed_numeric = pbio_differentiator_get_speed(&obs->differentiator, angle);

    // Apply observer error feedback as voltage.
    int32_t feedback_voltage = pbio_observer_get_feedback_voltage(obs, angle);

    // Check stall condition.
    update_stall_state(obs, time, actuation, voltage, feedback_voltage);

    // The observer will get the applied voltage plus the feedback voltage to
    // keep it in sync with the real system.
    voltage += feedback_voltage;

    // The only modeled torque is a static friction torque.
    int32_t torque = obs->speed > 0 ? m->torque_friction / 2: -m->torque_friction / 2;

    // Get next state based on current state and input: x(k+1) = Ax(k) + Bu(k)
    pbio_angle_add_mdeg(&obs->angle,
        PRESCALE_SPEED * obs->speed / m->d_angle_d_speed +
        PRESCALE_CURRENT * obs->current / m->d_angle_d_current +
        PRESCALE_VOLTAGE * voltage / m->d_angle_d_voltage +
        PRESCALE_TORQUE * torque / m->d_angle_d_torque);
    int32_t speed_next = pbio_int_math_clamp(0 +
        PRESCALE_SPEED * obs->speed / m->d_speed_d_speed +
        PRESCALE_CURRENT * obs->current / m->d_speed_d_current +
        PRESCALE_VOLTAGE * voltage / m->d_speed_d_voltage +
        PRESCALE_TORQUE * torque / m->d_speed_d_torque, MAX_NUM_SPEED);
    int32_t current_next = pbio_int_math_clamp(0 +
        PRESCALE_SPEED * obs->speed / m->d_current_d_speed +
        PRESCALE_CURRENT * obs->current / m->d_current_d_current +
        PRESCALE_VOLTAGE * voltage / m->d_current_d_voltage +
        PRESCALE_TORQUE * torque / m->d_current_d_torque, MAX_NUM_CURRENT);

    // TODO: Better friction model.
    if ((speed_next < 0) != (speed_next - PRESCALE_TORQUE * torque / m->d_speed_d_torque < 0)) {
        speed_next = 0;
    }

    // Save new state.
    obs->speed = speed_next;
    obs->current = current_next;
}

/**
 * Checks whether system is stalled by testing how far the estimate is ahead of
 * the measured angle, which is a measure for an unmodeled load.
 *
 * @param [in]  obs             The observer instance.
 * @param [in]  time            Wall time.
 * @param [out] stall_duration  For how long it has been stalled.
 * @return                      True if stalled, false if not.
 */
bool pbio_observer_is_stalled(const pbio_observer_t *obs, uint32_t time, uint32_t *stall_duration) {
    // Return stall flag, if stalled for some time.
    if (obs->stalled && time - obs->stall_start > obs->settings.stall_time) {
        *stall_duration = time - obs->stall_start;
        return true;
    }
    *stall_duration = 0;
    return false;
}

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref) {

    int32_t friction_compensation_torque = model->torque_friction / 2 * pbio_int_math_sign(rate_ref);
    int32_t back_emf_compensation_torque = PRESCALE_SPEED * pbio_int_math_clamp(rate_ref, MAX_NUM_SPEED) / model->d_torque_d_speed;
    int32_t acceleration_torque = PRESCALE_ACCELERATION * pbio_int_math_clamp(acceleration_ref, MAX_NUM_ACCELERATION) / model->d_torque_d_acceleration;

    // Total feedforward torque
    return pbio_int_math_clamp(friction_compensation_torque + back_emf_compensation_torque + acceleration_torque, MAX_NUM_TORQUE);
}

int32_t pbio_observer_torque_to_voltage(const pbio_observer_model_t *model, int32_t desired_torque) {
    return PRESCALE_TORQUE * pbio_int_math_clamp(desired_torque, MAX_NUM_TORQUE) / model->d_voltage_d_torque;
}

int32_t pbio_observer_voltage_to_torque(const pbio_observer_model_t *model, int32_t voltage) {
    return PRESCALE_VOLTAGE * pbio_int_math_clamp(voltage, MAX_NUM_VOLTAGE) / model->d_torque_d_voltage;
}
