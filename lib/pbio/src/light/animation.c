// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHT

#include <assert.h>
#include <stdbool.h>

#include <contiki.h>

#include <pbio/util.h>

#include "animation.h"

/**
 * This is used as a value for the next_animation field to indicate when an
 * animation is stopped.
 */
#define PBIO_LIGHT_ANIMATION_STOPPED ((pbio_light_animation_t *)1)

PROCESS(pbio_light_animation_process, "light animation");
static pbio_light_animation_t *pbio_light_animation_list_head;

/**
 * Initializes required fields of an animation data structure.
 * @param [in]  animation       The animation instance
 * @param [in]  next            The animation update callback
 */
void pbio_light_animation_init(pbio_light_animation_t *animation, pbio_light_animation_next_t next) {
    animation->next = next;
    animation->next_animation = PBIO_LIGHT_ANIMATION_STOPPED;
}

/**
 * Starts an animation in the background.
 *
 * The animation instance must have been initialized with pbio_light_animation_init().
 *
 * The animation must be stopped with pbio_light_animation_stop() before calling
 * pbio_light_animation_start() again.
 *
 * @param [in] animation    The animation instance.
 */
void pbio_light_animation_start(pbio_light_animation_t *animation) {
    assert(animation->next_animation == PBIO_LIGHT_ANIMATION_STOPPED);

    animation->next_animation = pbio_light_animation_list_head;
    pbio_light_animation_list_head = animation;

    process_start(&pbio_light_animation_process);

    PROCESS_CONTEXT_BEGIN(&pbio_light_animation_process);
    etimer_set(&animation->timer, 0);
    PROCESS_CONTEXT_END(&pbio_light_animation_process);

    assert(animation->next_animation != PBIO_LIGHT_ANIMATION_STOPPED);
}

/**
 * Stops an animation.
 *
 * This must be called once for each call to pbio_light_animation_start().
 *
 * @param [in] animation    The animation instance.
 */
void pbio_light_animation_stop(pbio_light_animation_t *animation) {
    assert(pbio_light_animation_list_head != NULL);
    assert(animation->next_animation != PBIO_LIGHT_ANIMATION_STOPPED);

    etimer_stop(&animation->timer);

    if (pbio_light_animation_list_head == animation) {
        pbio_light_animation_list_head = animation->next_animation;
        if (pbio_light_animation_list_head == NULL) {
            process_exit(&pbio_light_animation_process);
        }
    } else {
        for (pbio_light_animation_t *a = pbio_light_animation_list_head; a != NULL; a = a->next_animation) {
            if (a->next_animation == animation) {
                a->next_animation = animation->next_animation;
                break;
            }
        }
    }

    animation->next_animation = PBIO_LIGHT_ANIMATION_STOPPED;
}

/**
 * Stops all animations.
 *
 * This is intended to stop all animations when a user program is interrupted
 * and pbio_light_animation_stop() will no longer be called.
 */
void pbio_light_animation_stop_all(void) {
    while (pbio_light_animation_list_head) {
        pbio_light_animation_stop(pbio_light_animation_list_head);
    }
}

/**
 * Tests if the animation has been started.
 *
 * @param [in] animation    The animation instance.
 * @return                  *true* if the animation is started, otherwise *false*.
 */
bool pbio_light_animation_is_started(pbio_light_animation_t *animation) {
    return animation->next_animation != PBIO_LIGHT_ANIMATION_STOPPED;
}

PROCESS_THREAD(pbio_light_animation_process, ev, data) {
    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
        struct etimer *timer = data;
        pbio_light_animation_t *animation = PBIO_CONTAINER_OF(timer, pbio_light_animation_t, timer);
        clock_time_t interval = animation->next(animation);
        if (pbio_light_animation_is_started(animation)) {
            etimer_reset_with_new_interval(&animation->timer, interval);
            /* If the timer is to fire in the past, restart it immediately instead. */
            if (clock_time() > etimer_expiration_time(&animation->timer)) {
                etimer_reset_with_new_interval(&animation->timer, 0);
                etimer_restart(&animation->timer);
            }
        }
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_LIGHT
