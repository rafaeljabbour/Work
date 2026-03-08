#include "pan_motor.h"

#include "config.h"

void panMotorSetDirection(PanDirection_t dir) {
  if (dir == PAN_DIR_CW) {
    // pulls line to 5V, which makes opto OFF, turning motor CW
    HAL_GPIO_WritePin(PAN_DIR_PORT, PAN_DIR_PIN, GPIO_PIN_SET);
  } else {
    // pulls line to LOW, which makes opto ON, turning motor CCW
    HAL_GPIO_WritePin(PAN_DIR_PORT, PAN_DIR_PIN, GPIO_PIN_RESET);
  }
}

void panMotorStart(uint32_t frequency) {
  // limit the frequency to 1-500000 Hz
  if (frequency < 1) frequency = 1;
  if (frequency > 500000) frequency = 500000;

  uint32_t period = (TIM2_COUNTER_CLOCK_HZ / frequency) - 1;
  if (period < 1) period = 1;

  uint32_t pulse = (period + 1) / 2;

  // set the period and pulse width using pwm
  __HAL_TIM_SET_AUTORELOAD(&htim2, period);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse);
  __HAL_TIM_SET_COUNTER(&htim2, 0);

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
}

void panMotorStop(void) { HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3); }

void panMotorSetFrequency(uint32_t frequency) {
  // limit the frequency to 1-500000 Hz
  if (frequency < 1) frequency = 1;
  if (frequency > 500000) frequency = 500000;

  uint32_t period = (TIM2_COUNTER_CLOCK_HZ / frequency) - 1;
  if (period < 1) period = 1;

  uint32_t pulse = (period + 1) / 2;

  // set the period and pulse width using pwm
  __HAL_TIM_SET_AUTORELOAD(&htim2, period);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse);
}