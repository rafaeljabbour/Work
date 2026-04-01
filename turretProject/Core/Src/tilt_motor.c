#include "tilt_motor.h"
#include "config.h"

extern TIM_HandleTypeDef htim3;

void tiltMotorSetDirection(TiltDirection_t dir) {
    if (dir == TILT_DIR_UP) {
        HAL_GPIO_WritePin(TILT_DIR_PORT, TILT_DIR_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(TILT_DIR_PORT, TILT_DIR_PIN, GPIO_PIN_SET);
    }
}

void tiltMotorStart(uint32_t frequency) {
    if (frequency < 1) frequency = 1;
    if (frequency > 500000) frequency = 500000;

    uint32_t period = (TIM3_COUNTER_CLOCK_HZ / frequency) - 1;
    if (period < 1) period = 1;

    uint32_t pulse = (period + 1) / 2;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void tiltMotorStop(void) {
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void tiltMotorSetFrequency(uint32_t frequency) {
    if (frequency < 1) frequency = 1;
    if (frequency > 500000) frequency = 500000;

    uint32_t period = (TIM3_COUNTER_CLOCK_HZ / frequency) - 1;
    if (period < 1) period = 1;

    uint32_t pulse = (period + 1) / 2;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}