/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"

#include "config.h"
#include "ov7670.h"

#include <stdio.h>
#include <string.h>



uint16_t snapshot_buff[IMG_ROWS * IMG_COLS];

volatile uint8_t dma_flag = 0;


void print_buf(void);
void test_print_buf(void);


int main(void)
{
  /* Reset of all peripherals */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C2_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();

  char msg[100];

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  ov7670_init();
  print_msg("Init done!\r\n");

  // Your start up code here

  // while (1)
  // {
  //   // Your code here
  //   if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin)) {
  //     HAL_Delay(100);  // debounce
  //     test_print_buf();
  //   }
  // }

  // while (1)
  // {
  //   // Your code here
  //   if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin)) {
  //     HAL_Delay(100);  // debounce
  //     ov7670_snapshot(snapshot_buff);
  //     int timeout = 5000;
  //     while (!dma_flag && timeout > 0) { HAL_Delay(1); timeout--; }
  //     if (dma_flag) {
  //       dma_flag = 0;
  //       print_buf();
  //     }
  //   }
  // }

  ov7670_capture(snapshot_buff);
  while (1)
  {
    // Wait for the DMA interrupt to flag that a full frame is ready
    while (!dma_flag) HAL_Delay(1);

    // Clear the flag for the next frame
    dma_flag = 0;

    // This stops the camera from overwriting the buffer while we are sending it
    HAL_DCMI_Suspend(&hdcmi);

    // Send the captured frame out over the Serial port
    print_buf();

    // Resume the DCMI hardware to capture the next frame
    HAL_DCMI_Resume(&hdcmi);
  }
}


void print_buf() {
  // Send image data through serial port.
  // Your code here
  print_msg("PREAMBLE!\r\n");
  uint8_t *buff = (uint8_t*)snapshot_buff;

  for (int i = 1; i < IMG_ROWS * IMG_COLS * 2; i += 2) {
    uart_send_bin(&buff[i], 1);
  }
  print_msg("!END!\r\n");
}

void test_print_buf() {
  print_msg("\r\nPREAMBLE!\r\n");
  uint8_t pixel = 0x00;
  for (int i = 0; i < IMG_ROWS * IMG_COLS; i++) {
    uart_send_bin(&pixel, 1);
  }
  print_msg("!END!\r\n");
}

