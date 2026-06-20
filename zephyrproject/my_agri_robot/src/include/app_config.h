#pragma once

#include <stdint.h>

/* ---- RTOS Task Configuration ---- */
#define MOTOR_TASK_STACK_SIZE       2048U
#define CMD_TASK_STACK_SIZE         2048U
#define MOTOR_TASK_PRIORITY         5
#define CMD_TASK_PRIORITY           3

/* ---- Message Queue ---- */
#define MOTOR_MSG_QUEUE_DEPTH       10U
#define CMD_MSG_QUEUE_DEPTH         10U

/* ---- UART / Ring Buffer ---- */
#define CMD_RING_BUF_SIZE           256U
#define CMD_UART_RX_BUF_SIZE        64U
#define CMD_UART_TIMEOUT_US         1000U

/* ---- Frame Protocol ---- */
#define FRAME_HEADER                0xAAU
#define FRAME_END                   0x55U
#define FRAME_SIZE                  7U     /* Header(1) ID(1) OP(1) Data(2) CRC(1) End(1) */
#define FRAME_IDX_HEADER            0U
#define FRAME_IDX_MOTOR_ID          1U
#define FRAME_IDX_OPCODE            2U
#define FRAME_IDX_DATA_HI           3U
#define FRAME_IDX_DATA_LO           4U
#define FRAME_IDX_CRC               5U
#define FRAME_IDX_END               6U

/* ---- Opcode definitions ---- */
#define OPCODE_SPE                  0x01U   /* Set speed */
#define OPCODE_DIR                  0x02U   /* Set direction */
#define OPCODE_STOP                 0x03U   /* Stop */
#define OPCODE_RESET                0x04U   /* Reset fault */

/* ---- Motor Timing ---- */
/* Wait after speed=0 before direction change (CL57C datasheet req: 50-100ms) */
#define MOTOR_DIR_CHANGE_WAIT_MS    75U
/* Minimum setup time between DIR change and first PUL (architecture: min 5us) */
#define MOTOR_DIR_SETUP_US          5U
/* Minimum pulse width for CL57C (datasheet: 2.5us min, use 5us margin) */
#define MOTOR_MIN_PULSE_NS          5000U
/* Acceleration step: speed increment per step (Hz) */
#define MOTOR_ACCEL_STEP_HZ         200U
/* Acceleration interval (ms between each speed step) */
#define MOTOR_ACCEL_INTERVAL_MS     10U
/* Maximum speed in Hz (CL57C: 200kHz max) */
#define MOTOR_MAX_SPEED_HZ          200000U

/* ---- Motor ID ---- */
#define MOTOR_ID_DEFAULT            0x01U
