
#ifndef _PBIO_MOTOR_H_
#define _PBIO_MOTOR_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

/**
 * Motor direction convention.
 */
typedef enum {
    PBIO_MOTOR_DIR_NORMAL,      /**< Use the normal motor-specific convention for the positive direction */
    PBIO_MOTOR_DIR_INVERTED,    /**< Swap positive and negative for both the encoder value and the duty cycle */
} pbio_motor_dir_t;

/**
 * Initializes the low level motor driver. This should be called only once and
 * must be called before using any other motor functions.
 */
void pbio_motor_init(void);

/**
 * Releases the low level motor driver. No motor functions can be called after
 * calling this function.
 */
void pbio_motor_deinit(void);

/**
 * Gets the tachometer encoder count.
 * @param [in]  port    The motor port
 * @param [out] count   The count
 * @return              ::PB_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count);

/**
 * Gets the tachometer encoder rate in counts per second.
 * @param [in]  port    The motor port
 * @param [out] rate    The rate
 * @return              ::PB_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *rate);

/**
 * Instructs the motor to coast freely.
 * @param [in]  port    The motor port
 * @return              ::PB_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_coast(pbio_port_t port);

/**
 * Sets the PWM duty cycle for the motor. Setting a duty cycle of 0 will "brake" the motor.
 * @param [in]  port        The motor port
 * @param [in]  duty_cycle  The duty cycle -10000 to 10000
 * @return                  ::PB_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if duty_cycle is out of range
 *                          ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle);

/**
 * Sets the direction of the motor. See ::pbio_motor_dir_t for more information.
 * @param [in]  port        The motor port
 * @param [in]  direction   The direction
 * @return                  ::PB_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if direction is not valid
 *                          ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_motor_set_direction(pbio_port_t port, pbio_motor_dir_t direction);

#endif // _PBIO_MOTOR_H_
