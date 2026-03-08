#include "target_stream.h"

#include <stdio.h>
#include <string.h>

#include "config.h"

extern uint16_t snapshot_buff[];
extern volatile uint8_t dma_flag;

// sends a colored image, with the bounding box and centroid drawn on it
void TargetStreamFrame(const uint16_t *frameBuf, const TargetResult_t *result) {
  const uint8_t *buff = (const uint8_t *)frameBuf;

  // 1. Send preamble
  print_msg("PREAMBLE!\r\n");

  // 2. Send full color YUV422 image (2 bytes per pixel) in one big chunk
  uart_send_bin((uint8_t *)buff, IMG_ROWS * IMG_COLS * 2);

  // 3. Send detection result
  char det[120];
  sprintf(det, "!DET:%u,%u,%u,%lu,%u,%u,%u,%u\r\n", result->detected,
          result->centroidRow, result->centroidCol,
          (unsigned long)result->pixelCount, result->bboxRowMin,
          result->bboxRowMax, result->bboxColMin, result->bboxColMax);
  print_msg(det);
}

void run_camera_stream(void) {
  TargetResult_t result;

  print_msg("Starting camera stream mode...\r\n");
  HAL_Delay(500); /* give Python script time to connect */

  /* Start continuous DMA capture */
  ov7670_capture(snapshot_buff);

  while (1) {
    /* Wait for DMA to signal a full frame is ready */
    while (!dma_flag) {
      HAL_Delay(1);
    }
    dma_flag = 0;

    /* Pause camera so it doesn't overwrite the buffer mid-processing */
    HAL_DCMI_Suspend(&hdcmi);

    /* Run detection */
    TargetDetect(snapshot_buff, &result);

    /* Send image + detection result to PC */
    TargetStreamFrame(snapshot_buff, &result);

    /* LED feedback */
    if (result.detected) {
      HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_SET);   /* green ON  */
      HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_RESET); /* red OFF   */
    } else {
      HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_RESET); /* green OFF */
      HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);   /* red ON    */
    }

    /* Resume camera for next frame */
    HAL_DCMI_Resume(&hdcmi);
  }
}