#ifndef __TURRET_FSM_H
#define __TURRET_FSM_H

#include <stdint.h>

#include "main.h"
#include "target_detect.h"

typedef enum {
  STATE_IDLE,       // we wait for B1 to be pressed to start
  STATE_SEARCHING,  // we sweep the camera to find the target
  STATE_AIMING,     // we aim the turret to the centroid
  STATE_LOCKED_ON,  // we are on target, ready to fire
  STATE_FIRING,     // we fire the sequence
} TurretState_t;

/*
 * MDrive motor is a 51,200 steps/rev.
 * We are running it at 3000 Hz, so we cover about 21 deg per second.
 * A full 180 deg sweep takes ~8.5 seconds.
 * We reverse after ~25,600 steps (180 deg).
 */
#define SEARCH_STEPS_BEFORE_REVERSE 25600
#define AIM_COL_TOLERANCE 20
#define AIM_ROW_TOLERANCE 20
#define LOCK_ON_FRAME_COUNT 3
#define SEARCH_SPEED_HZ 300
#define AIM_PAN_GAIN 3
#define AIM_TILT_GAIN 3
#define TARGET_LOST_FRAMES 10

typedef struct {
  TurretState_t state;      // current state
  TurretState_t prevState;  // previous state

  // tells us how far off we are from the center of the target
  int16_t errorCol;  // positive val means target is right of camera's center
  int16_t errorRow;  // positive val means target is below camera's center

  uint8_t lockOnCount;  // number of frames we have been on target
  uint8_t lostCount;    // number of frames we haven't detected a target

  // the direction we are sweeping the camera in (0 = CW, 1 = CCW)
  uint8_t searchDir;

  uint32_t searchStepCount;  // how much we spun in one direction
  uint32_t frameCount;  // how many frames we have processed since we started
  uint32_t lockOnStartTick;  // HAL tick when we entered LOCKED_ON
} TurretFSM_t;

#define LOCKED_ON_AUTO_FIRE_MS 5000

void TurretFsmInit(void);
void TurretFsmUpdate(const TargetResult_t* result);
TurretState_t TurretFsmGetState(void);

static inline const char* TurretFsmStateName(TurretState_t state) {
  switch (state) {
    case STATE_IDLE:
      return "IDLE";
    case STATE_SEARCHING:
      return "SEARCHING";
    case STATE_AIMING:
      return "AIMING";
    case STATE_LOCKED_ON:
      return "LOCKED_ON";
    case STATE_FIRING:
      return "FIRING";
    default:
      return "UNKNOWN";
  }
}

#endif