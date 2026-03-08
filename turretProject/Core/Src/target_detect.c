#include "target_detect.h"

#include <stdio.h>
#include <string.h>

#include "config.h"

// main detection function
void TargetDetect(const uint16_t *frameBuf, TargetResult_t *result) {
  // we cast to byte it so we can access individual Y/Cb/Cr bytes
  const uint8_t *buff = (const uint8_t *)frameBuf;

  // the accumulators for the centroid calculation
  uint32_t sumRow = 0;
  uint32_t sumCol = 0;
  uint32_t count = 0;

  // the bounding box trackers shrink as we find red pixels
  uint16_t bboxRowMin = IMG_ROWS;
  uint16_t bboxRowMax = 0;
  uint16_t bboxColMin = IMG_COLS;
  uint16_t bboxColMax = 0;

  // we iterate over every pixel pair in the frame
  for (uint16_t row = 0; row < IMG_ROWS; row++) {
    for (uint16_t col = 0; col < IMG_COLS - 1; col += 2) {
      // gives us where the pixel pair starts in the buffer
      uint32_t base = ((uint32_t)row * IMG_COLS + col) * 2;

      uint8_t cb = buff[base + 0];
      uint8_t cr = buff[base + 2];

      // if the pixel pair is red, we add both to the centroid accumulator
      if (cr >= CR_THRESHOLD_MIN && cb <= CB_THRESHOLD_MAX) {
        // left pixel (row, col)
        sumRow += row;
        sumCol += col;

        // right pixel (row, col+1)
        sumRow += row;
        sumCol += (col + 1);

        count += 2;

        // update the bounding box
        if (row < bboxRowMin) bboxRowMin = row;
        if (row > bboxRowMax) bboxRowMax = row;
        if (col < bboxColMin) bboxColMin = col;
        if ((col + 1) > bboxColMax) bboxColMax = col + 1;
      }
    }
  }

  // fill in the result struct
  result->pixelCount = count;

  if (count >= MIN_RED_PIXELS) {
    result->detected = 1;
    result->centroidRow = (uint16_t)(sumRow / count);
    result->centroidCol = (uint16_t)(sumCol / count);
    result->bboxRowMin = bboxRowMin;
    result->bboxRowMax = bboxRowMax;
    result->bboxColMin = bboxColMin;
    result->bboxColMax = bboxColMax;
  } else {
    result->detected = 0;
    result->centroidRow = 0;
    result->centroidCol = 0;
    result->bboxRowMin = 0;
    result->bboxRowMax = 0;
    result->bboxColMin = 0;
    result->bboxColMax = 0;
  }
}

// UART debug output
void TargetPrintResult(const TargetResult_t *result) {
  char msg[200];

  if (result->detected) {
    sprintf(msg, "TARGET: row=%u col=%u px=%lu bbox=[%u..%u]x[%u..%u]\r\n",
            result->centroidRow, result->centroidCol,
            (unsigned long)result->pixelCount, result->bboxRowMin,
            result->bboxRowMax, result->bboxColMin, result->bboxColMax);
  } else {
    sprintf(msg, "NO TARGET (px=%lu)\r\n", (unsigned long)result->pixelCount);
  }

  print_msg(msg);
}

// print raw Cr/Cb for a specific location
void TargetDebugPixel(const uint16_t *frameBuf, uint16_t row, uint16_t col) {
  const uint8_t *buff = (const uint8_t *)frameBuf;

  // round column down to even so we land on a pair boundary
  col &= ~1u;

  if (row >= IMG_ROWS || col >= IMG_COLS) {
    print_msg("TargetDebugPixel: out of bounds\r\n");
    return;
  }

  uint32_t base = ((uint32_t)row * IMG_COLS + col) * 2;

  uint8_t cb = buff[base + 0];
  uint8_t y0 = buff[base + 1];
  uint8_t cr = buff[base + 2];
  uint8_t y1 = buff[base + 3];

  char msg[120];
  sprintf(msg, "PIX(%u,%u): Cb=%u Y0=%u Cr=%u Y1=%u  %s\r\n", row, col, cb, y0,
          cr, y1,
          (cr >= CR_THRESHOLD_MIN && cb <= CB_THRESHOLD_MAX) ? "RED" : "---");
  print_msg(msg);
}
