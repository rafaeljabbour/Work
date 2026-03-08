/*
 * The FSM runs once per camera frame
 *
 * Motor control strategy:
 *   - IDLE: waiting for B1 to be pressed to start searching
 *   - SEARCHING: keep sweeping left and right until a target is found
 *   - AIMING: once a target is found, use proportional control to centerthe
 *     target in the camera's center by moving the pan motor
 *   - LOCKEDON: motors stopped, waiting for B1 to be pressed to fire
 *   - FIRING: firing timeeeeeee!
 */

#include "turret_fsm.h"

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "ov7670.h"
#include "pan_motor.h"

// the FSM context
static TurretFSM_t fsm;

// helpers
static void FsmEnterState(TurretState_t new_state);
static void FsmHandleIdle(const TargetResult_t *result);
static void FsmHandleSearching(const TargetResult_t *result);
static void FsmHandleAiming(const TargetResult_t *result);
static void FsmHandleLockedOn(const TargetResult_t *result);
static void FsmHandleFiring(const TargetResult_t *result);

void TurretFsmInit(void) {
  // set default values for the FSM
  memset(&fsm, 0, sizeof(fsm));
  // set the initial and previous state to IDLE
  fsm.state = STATE_IDLE;
  fsm.prevState = STATE_IDLE;
  // we start searching clockwise
  fsm.searchDir = 0;

  // we make sure motors are stopped
  pan_motor_stop();

  print_msg("(initialized) -> (IDLE)\r\n");
  print_msg("press B1 to start searching\r\n");
}

static void FsmEnterState(TurretState_t newState) {
  // we save previous and update current state
  fsm.prevState = fsm.state;
  fsm.state = newState;

  // we reset the counters for the new state
  fsm.lockOnCount = 0;
  fsm.lostCount = 0;

  // reset LED indicators
  HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_RESET);

  // we log the transition, so we know what happened
  // TODO: make it display on the LCD
  char msg[80];
  sprintf(msg, "(%s) -> (%s)\r\n", TurretFsmStateName(fsm.prevState),
          TurretFsmStateName(newState));
  print_msg(msg);

  switch (newState) {
    case STATE_IDLE:
      // LD1 will blink in the handler
      break;
    case STATE_SEARCHING:
      HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_SET);  // blue
      fsm.searchStepCount = 0;
      break;
    case STATE_AIMING:
      HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_SET);  // green
      break;
    case STATE_LOCKED_ON:
      HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_SET);  // green
      HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);  // red
      break;
    case STATE_FIRING:
      HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);  // red
      break;
  }
}

void TurretFsmUpdate(const TargetResult_t *result) {
  // the FSM update gets called once per camera frame
  fsm.frameCount++;

  // error between the detected box around the target and the camera's center
  if (result->detected) {
    fsm.errorCol = (int16_t)result->centroidCol - (IMG_COLS / 2);
    fsm.errorRow = (int16_t)result->centroidRow - (IMG_ROWS / 2);
  }

  // we just jump to the current state handler
  switch (fsm.state) {
    case STATE_IDLE:
      FsmHandleIdle(result);
      break;
    case STATE_SEARCHING:
      FsmHandleSearching(result);
      break;
    case STATE_AIMING:
      FsmHandleAiming(result);
      break;
    case STATE_LOCKED_ON:
      FsmHandleLockedOn(result);
      break;
    case STATE_FIRING:
      FsmHandleFiring(result);
      break;
  }
}

// the handles that handle each individual state

// IDLE state handler
static void FsmHandleIdle(const TargetResult_t *result) {
  // we don't actually use this but to be consistent with the other handlers
  (void)result;

  // blink green LED every 15 frames
  if (fsm.frameCount % 15 == 0) {
    HAL_GPIO_TogglePin(GPIOB, LD1_Pin);
  }

  // if B1 is pressed, we start searching
  if (isButtonPressed()) {
    HAL_Delay(100);  // debounce
    FsmEnterState(STATE_SEARCHING);
  }
}

// SEARCHING state handler
static void FsmHandleSearching(const TargetResult_t *result) {
  // if a target is detected, we stop the motor and go to aiming
  if (result->detected) {
    pan_motor_stop();
    fsm.lostCount = 0;
    FsmEnterState(STATE_AIMING);
    return;
  }

  fsm.searchStepCount++;

  if (fsm.searchStepCount >= SEARCH_STEPS_BEFORE_REVERSE) {
    // reverse direction
    pan_motor_stop();
    HAL_Delay(100);

    // toggle direction
    fsm.searchDir = !fsm.searchDir;
    fsm.searchStepCount = 0;
  }

  if (fsm.searchStepCount <= 1) {
    // start the motor at start or when we reverse
    if (fsm.searchDir) {
      pan_motor_set_direction(PAN_DIR_CCW);
    } else {
      pan_motor_set_direction(PAN_DIR_CW);
    }
    pan_motor_start(SEARCH_SPEED_HZ);
  }

  // if we press B1 during search, go back to IDLE
  if (isButtonPressed()) {
    HAL_Delay(100);  // debounce
    pan_motor_stop();
    FsmEnterState(STATE_IDLE);
  }
}

// AIMING state handler
static void FsmHandleAiming(const TargetResult_t *result) {
  if (!result->detected) {
    // we track how many frames we have been without a target
    fsm.lostCount++;
    // if we have been without a target for too long, we go back to searching
    if (fsm.lostCount >= TARGET_LOST_FRAMES) {
      pan_motor_stop();
      // we reset the lost and lock on counters
      fsm.lostCount = 0;
      fsm.lockOnCount = 0;

      FsmEnterState(STATE_SEARCHING);
    }
    return;
  }

  // we reset the lost counter when we see a target
  fsm.lostCount = 0;

  // this is the main logic for aiming the turret

  int16_t errorCol = fsm.errorCol;
  // if the target is to the right of the center, we pan clockwise
  if (errorCol > AIM_COL_TOLERANCE) {
    pan_motor_set_direction(PAN_DIR_CW);
    // faster when target is further away and slower when closer to the center
    uint32_t speed = (uint32_t)(errorCol * AIM_PAN_GAIN);
    if (speed < 500) speed = 500;      // minimum speed
    if (speed > 15000) speed = 15000;  // maximum speed
    pan_motor_start(speed);

  }
  // if the target is to the left of the center, we pan counter-clockwise
  else if (errorCol < -AIM_COL_TOLERANCE) {
    pan_motor_set_direction(PAN_DIR_CCW);
    // faster when target is further away and slower when closer to the center
    uint32_t speed = (uint32_t)(-errorCol * AIM_PAN_GAIN);
    if (speed < 500) speed = 500;      // minimum speed
    if (speed > 15000) speed = 15000;  // maximum speed
    pan_motor_start(speed);
  }
  // if the target is within the horizontal tolerance, we stop the motor
  else {
    pan_motor_stop();
  }

  // TODO: Add tilt motor control when TMC2209 driver arrives.

  int16_t errorRow = fsm.errorRow;

  // check if on target
  uint8_t colOk =
      (errorCol >= -AIM_COL_TOLERANCE && errorCol <= AIM_COL_TOLERANCE);
  // TODO: uint8_t rowOk = (errorRow >= -AIM_ROW_TOLERANCE && errorRow <=
  // AIM_ROW_TOLERANCE);
  uint8_t rowOk = 1;

  if (colOk && rowOk) {
    // we track how many frames we have been on target
    fsm.lockOnCount++;
    // if we have been on target for too long, we go to locked on
    if (fsm.lockOnCount >= LOCK_ON_FRAME_COUNT) {
      pan_motor_stop();
      FsmEnterState(STATE_LOCKED_ON);
    }
  } else {
    fsm.lockOnCount = 0;
  }

  // print aiming status every few frames
  if (fsm.frameCount % 5 == 0) {
    char msg[120];
    sprintf(msg, "AIM: errC=%d errR=%d lock=%u/%u\r\n", errorCol, errorRow,
            fsm.lockOnCount, LOCK_ON_FRAME_COUNT);
    print_msg(msg);
  }
}

// LOCKED_ON state handler
static void FsmHandleLockedOn(const TargetResult_t *result) {
  if (!result->detected) {
    fsm.lostCount++;
    if (fsm.lostCount >= TARGET_LOST_FRAMES) {
      fsm.lostCount = 0;
      fsm.lockOnCount = 0;
      FsmEnterState(STATE_AIMING);
    }
    return;
  }

  // we check if target drifted out of tolerance
  int16_t errorCol = fsm.errorCol;
  uint8_t colOk =
      (errorCol >= -AIM_COL_TOLERANCE && errorCol <= AIM_COL_TOLERANCE);

  if (!colOk) {
    // target moved, go back to aiming
    fsm.lockOnCount = 0;
    FsmEnterState(STATE_AIMING);
    return;
  }

  // still locked, reset lost counter
  fsm.lostCount = 0;

  // print lock status occasionally
  if (fsm.frameCount % 15 == 0) {
    char msg[80];
    sprintf(msg, "LOCKED: errC=%d errR=%d — press B1 to fire\r\n", errorCol,
            errorRow);
    print_msg(msg);
  }

  // if we press B1, we go to firing
  if (isButtonPressed()) {
    HAL_Delay(100);  // debounce
    FsmEnterState(STATE_FIRING);
  }
}

// FIRING state handler
static void FsmHandleFiring(const TargetResult_t *result) {
  (void)result;

  print_msg("FIRING TIME!\r\n");

  // TODO: Replace with actual firing mechanism control.

  for (int i = 0; i < 6; i++) {
    HAL_GPIO_TogglePin(GPIOB, LD3_Pin);
    HAL_Delay(150);
  }

  print_msg("TARGET ACQUIRED! Returning to IDLE.\r\n");

  // reset and go back to IDLE for the next target
  FsmEnterState(STATE_IDLE);
}