/*
 * MDrive 23 Plus pan motor control via Step/Direction interface
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

#define TIM2_COUNTER_CLOCK_HZ 1000000U  // 84 MHz / (83 [prescaler] + 1) = 1 MHz

typedef enum { PAN_DIR_CW = 0, PAN_DIR_CCW = 1 } PanDirection_t;

void panMotorSetDirection(PanDirection_t dir);
void panMotorStart(uint32_t frequency);
void panMotorStop(void);
void panMotorSetFrequency(uint32_t frequency);

#endif