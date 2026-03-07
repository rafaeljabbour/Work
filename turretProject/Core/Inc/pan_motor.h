/**
 * @file    pan_motor.h
 * @brief   MDrive 23 Plus pan motor control via Step/Direction interface
 *
 * Pins:
 *   PB10 (TIM2_CH3, Open-Drain): used to connect to MDrive 'Step Clock'
 *   PB11 (GPIO Open-Drain): used to connect to MDrive 'Direction'
 *   5V (CN8 pin 9): used to connect to MDrive 'Opto Reference'
 */

#ifndef __PAN_MOTOR_H
#define __PAN_MOTOR_H

#include "main.h"

#define PAN_STEP_PORT GPIOB
#define PAN_STEP_PIN GPIO_PIN_10

#define PAN_DIR_PORT GPIOB
#define PAN_DIR_PIN GPIO_PIN_11

typedef enum {
    PAN_DIR_CW  = 0,
    PAN_DIR_CCW = 1
} PanDirection_t;

void pan_motor_set_direction(PanDirection_t dir);
void pan_motor_start(uint32_t freq_hz);
void pan_motor_stop(void);
void pan_motor_set_speed(uint32_t freq_hz);

#endif