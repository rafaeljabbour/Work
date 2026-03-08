/*
 * 4 bytes equals to 2 pixels
 * eg: [Cb0][Y0][Cr0][Y1] [Cb2][Y2][Cr2][Y3]
 * pixel 1: [Cb0][Y0][Cr0]
 * pixel 2: [Cb0][Y1][Cr0]
 * pixel 3: [Cb2][Y2][Cr2]
 * pixel 4: [Cb2][Y3][Cr2]
 */

#ifndef __TARGET_DETECT_H
#define __TARGET_DETECT_H

#include <stdint.h>

#include "main.h"
#include "ov7670.h"

// we threshold to detect red objects
#define CR_THRESHOLD_MIN 160  // Cr must be ABOVE to be target
#define CB_THRESHOLD_MAX 115  // Cb must be BELOW to be target
#define MIN_RED_PIXELS 20     // minimum red pixels to count as a target

// Detection result
typedef struct {
  uint8_t detected;  // 1 if a red target was found, 0 otherwise
  // the centroid of the target in pixel coordinates
  uint16_t centroidRow;  // row of centroid
  uint16_t centroidCol;  // column of centroid
  uint32_t pixelCount;   // number of red pixels found
  uint16_t bboxRowMin;   // top row
  uint16_t bboxRowMax;   // bottom row
  uint16_t bboxColMin;   // left column
  uint16_t bboxColMax;   // right column
} TargetResult_t;

void TargetDetect(const uint16_t *frameBuf, TargetResult_t *result);
void TargetPrintResult(const TargetResult_t *result);
void TargetDebugPixel(const uint16_t *frameBuf, uint16_t row, uint16_t col);
void TargetDebugCrHistogram(const uint16_t *frameBuf);

#endif