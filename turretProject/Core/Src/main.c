/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include "main.h"

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "turret_fsm.h"
uint16_t snapshot_buff[IMG_ROWS * IMG_COLS];

volatile uint8_t dma_flag = 0;
void runTurretFsm(void);

int main(void) {
  // initialize the hardware
  HAL_Init();
  // configure the system clock
  SystemClock_Config();
  // initialize all configured peripherals
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C2_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  MX_TIM2_Init();

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  if (ov7670_init() != 0) {
    print_msg("ERROR: Camera init failed!\r\n");
    while (1) {
      HAL_Delay(1000);
    }
  }

  runTurretFsm();

  // run_camera_stream();

  while (1) {
  }
}

void runTurretFsm(void) {
  TargetResult_t result;

  print_msg("=== TURRET FSM MODE ===\r\n");

  // Initialize the FSM (starts in IDLE)
  TurretFsmInit();

  // Start continuous camera capture
  ov7670_capture(snapshot_buff);

  while (1) {
    // Wait for the next frame from DMA
    while (!dma_flag) {
      HAL_Delay(1);
    }
    dma_flag = 0;

    // Pause camera while we process
    HAL_DCMI_Suspend(&hdcmi);

    // Run red target detection
    TargetDetect(snapshot_buff, &result);

    // Feed the result into the FSM — this drives the motors
    TurretFsmUpdate(&result);

    // Resume camera capture
    HAL_DCMI_Resume(&hdcmi);
  }
}