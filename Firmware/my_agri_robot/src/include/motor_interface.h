#pragma once

#include <stdint.h>

/** Motor driver ID (maps to CL57C instance) */
typedef uint8_t MotorID_t;

/** Motor operational states - follows INIT->READY->RUNNING->STOP->FAULT */
typedef enum : uint8_t {
    MOTOR_STATE_INIT    = 0,
    MOTOR_STATE_READY   = 1,
    MOTOR_STATE_RUNNING = 2,
    MOTOR_STATE_STOP    = 3,
    MOTOR_STATE_FAULT   = 4,
} MotorState_t;

/** Motor rotation direction */
typedef enum : uint8_t {
    MOTOR_DIR_FORWARD  = 0,  /* PE2 LOW  */
    MOTOR_DIR_BACKWARD = 1,  /* PE2 HIGH */
} MotorDirection_t;

/** Motor speed in Hz (pulse frequency to step driver) */
typedef uint32_t MotorSpeed_t;

/**
 * @brief Inter-task message for motor control.
 *        Placed in k_msgq between cmd_task -> motor_task.
 */
typedef struct {
    MotorID_t       ID;         /**< Target motor driver ID */
    MotorState_t    State;      /**< Desired state command */
    MotorDirection_t Direction; /**< Desired direction */
    MotorSpeed_t    Speed;      /**< Desired speed in Hz (0 = stop) */
} MotorMessage_t;

/** Events internal to motor_sm (state machine events) */
typedef enum : uint8_t {
    MOTOR_EVT_INIT_OK   = 0,   /**< Health check passed */
    MOTOR_EVT_INIT_FAIL = 1,   /**< Health check failed */
    MOTOR_EVT_CMD_RUN   = 2,   /**< Run command received */
    MOTOR_EVT_CMD_STOP  = 3,   /**< Stop command received */
    MOTOR_EVT_STOPPED   = 4,   /**< Motor fully decelerated to zero */
    MOTOR_EVT_FAULT     = 5,   /**< Fault detected (ALM signal or driver error) */
    MOTOR_EVT_RESET     = 6,   /**< Manual reset from master */
} MotorEvent_t;
