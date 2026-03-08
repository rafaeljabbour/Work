/*
 * Protocol:
 * 1. "PREAMBLE!\r\n": marks the start of a new frame so the receiver can sync
 * 2. sends the raw YUV422 color bytes (144x174 x 2 bytes)
 * 3. "!DET:<detected>,<row>,<col>,<px>,<rmin>,<rmax>,<cmin>,<cmax>\r\n" : sends
 * the information on whether target was detected and it's location
 */

#ifndef __TARGET_STREAM_H
#define __TARGET_STREAM_H

#include <stdint.h>

#include "main.h"
#include "ov7670.h"
#include "target_detect.h"

void TargetStreamFrame(const uint16_t *frameBuf, const TargetResult_t *result);

#endif