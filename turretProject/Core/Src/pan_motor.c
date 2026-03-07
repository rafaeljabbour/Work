/**
 * @file    pan_motor.c
 * @brief   MDrive 23 Plus pan motor control implementation
 *
 * Step pulses: PB10 via TIM2_CH3 (Open-Drain)
 * Direction: PB11 via GPIO (Open-Drain)
 */

#include "pan_motor.h"
#include "config.h"

#define TIM2_COUNTER_CLOCK_HZ   1000000U   // 84 MHz / (83 [prescaler] + 1) = 1 MHz

void pan_motor_set_direction(PanDirection_t dir)
{
    if (dir == PAN_DIR_CW) {
        // release line (high-Z) pull-up to 5V, which makes opto OFF, turning motor CW
        HAL_GPIO_WritePin(PAN_DIR_PORT, PAN_DIR_PIN, GPIO_PIN_SET);
    } else {
        // pull line to LOW, which makes opto ON, turning motor CCW
        HAL_GPIO_WritePin(PAN_DIR_PORT, PAN_DIR_PIN, GPIO_PIN_RESET);
    }
}

void pan_motor_start(uint32_t freq_hz)
{
    if (freq_hz < 1) freq_hz = 1;
    if (freq_hz > 500000) freq_hz = 500000;

    uint32_t period = (TIM2_COUNTER_CLOCK_HZ / freq_hz) - 1;
    if (period < 1) period = 1;

    uint32_t pulse = (period + 1) / 2;

    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse);
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
}

void pan_motor_stop(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
}

void pan_motor_set_speed(uint32_t freq_hz)
{
    if (freq_hz < 1) freq_hz = 1;
    if (freq_hz > 500000) freq_hz = 500000;

    uint32_t period = (TIM2_COUNTER_CLOCK_HZ / freq_hz) - 1;
    if (period < 1) period = 1;

    uint32_t pulse = (period + 1) / 2;

    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse);
}