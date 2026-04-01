/*
 * NEMA 17 and TMC2209 tilt motor 
 *
 * Unlike the MDrive pan motor which needs open-drain GPIOs for 5V
 * optocoupled inputs, the TMC2209 accepts 3.3V logic directly,
 * so we use normal push-pull GPIOs.
 *
 * Pins:
 *   PB4 (TIM3_CH1, Push-Pull AF): Step pulse output
 *   PB5 (GPIO Push-Pull):         Direction output
 */

#ifndef __TILT_MOTOR_H
#define __TILT_MOTOR_H

#include "main.h"

#define TILT_STEP_PORT GPIOB
#define TILT_STEP_PIN  GPIO_PIN_4

#define TILT_DIR_PORT  GPIOB
#define TILT_DIR_PIN   GPIO_PIN_5

#define TIM3_COUNTER_CLOCK_HZ 1000000U

// TMC2209 has a default 1600 steps/rev
#define TILT_STEPS_PER_REV 1600U

typedef enum { TILT_DIR_UP = 0, TILT_DIR_DOWN = 1 } TiltDirection_t;

void tiltMotorSetDirection(TiltDirection_t dir);
void tiltMotorStart(uint32_t frequency);
void tiltMotorStop(void);
void tiltMotorSetFrequency(uint32_t frequency);

#endif