#pragma once

#include <stdint.h>

/* ---- RTOS Task Configuration ---- */
#define MOTOR_TASK_STACK_SIZE 2048U
#define CMD_TASK_STACK_SIZE 2048U
#define MOTOR_TASK_PRIORITY 5
#define CMD_TASK_PRIORITY 3

/* ---- Message Queue ---- */
#define MOTOR_MSG_QUEUE_DEPTH 10U
#define CMD_MSG_QUEUE_DEPTH 10U

/* ---- UART / Ring Buffer ---- */
#define CMD_RING_BUF_SIZE 256U
#define CMD_UART_RX_BUF_SIZE 64U
#define CMD_UART_TIMEOUT_US 1000U

/* ---- Frame Protocol ---- */
#define FRAME_HEADER 0xAAU
#define FRAME_END 0x55U
#define FRAME_SIZE 8U /* Header(1) SeqID(1) MotorID(1) OP(1) Data(2) CRC(1) End(1) */
#define FRAME_IDX_HEADER   0U
#define FRAME_IDX_SEQ_ID   1U
#define FRAME_IDX_MOTOR_ID 2U
#define FRAME_IDX_OPCODE   3U
#define FRAME_IDX_DATA_HI  4U
#define FRAME_IDX_DATA_LO  5U
#define FRAME_IDX_CRC      6U
#define FRAME_IDX_END      7U

/* ---- Opcode definitions ---- */
#define OPCODE_SPE 0x01U   /* Set speed */
#define OPCODE_DIR 0x02U   /* Set direction */
#define OPCODE_STOP 0x03U  /* Stop */
#define OPCODE_RESET 0x04U /* Reset fault */

/* ---- Motor Timing ---- */
/* Wait after speed=0 before direction change (CL57C datasheet req: 50-100ms) */
#define MOTOR_DIR_CHANGE_WAIT_MS 75U
/* Minimum setup time between DIR change and first PUL (architecture: min 5us) */
#define MOTOR_DIR_SETUP_US 5U
/* Minimum pulse width for CL57C (datasheet: 2.5us min, use 5us margin) */
#define MOTOR_MIN_PULSE_NS 5000U
/* Acceleration step: speed increment per step (Hz) */
#define MOTOR_ACCEL_STEP_HZ 200U
/* Acceleration interval (ms between each speed step) */
#define MOTOR_ACCEL_INTERVAL_MS 10U
/* Maximum pulse frequency supported by CL57C hardware (datasheet: 200kHz) */
#define MOTOR_MAX_SPEED_HZ 150000U
/* Maximum speed accepted from master (software limit, architecture rule) */
#define MOTOR_MAX_SPEED_RPM 5000U

/* ---- Motor Pulses Per Revolution ---- */
/* Default microstepping: 1600 pulses/rev (CL57C SW setting).
 * Hz = RPM x MOTOR_PULSES_PER_REV / 60 */
#define MOTOR_PULSES_PER_REV 1600U

/* ---- Motor ID ---- */
#define MOTOR_ID_DEFAULT 0x01U

/* ---- Notify Frame (slave → master, 10 bytes) ---- */
#define NOTIFY_FRAME_SIZE        10U
#define NOTIFY_IDX_HEADER        0U
#define NOTIFY_IDX_SEQ_ID        1U
#define NOTIFY_IDX_MOTOR_ID      2U
#define NOTIFY_IDX_OPCODE        3U
#define NOTIFY_IDX_STATE         4U
#define NOTIFY_IDX_SPEED_HI      5U
#define NOTIFY_IDX_SPEED_LO      6U
#define NOTIFY_IDX_FAULT_CODE    7U
#define NOTIFY_IDX_CRC           8U
#define NOTIFY_IDX_END           9U

/* Notify opcodes */
#define NOTIFY_OPCODE_STATUS     0x80U   /* Periodic status (state + speed) */
#define NOTIFY_OPCODE_FAULT      0x82U   /* Emergency fault notification */

/* Fault codes (embedded in FaultCode byte of notify frame) */
#define FAULT_CODE_NONE          0x00U
#define FAULT_CODE_ALM           0x01U   /* CL57C ALM signal triggered */
#define FAULT_CODE_QUEUE_FULL    0x02U   /* motor_task command queue full */
#define FAULT_CODE_HEALTH_FAIL   0x03U   /* Health check failed at init/reset */

/* Notify queue depth (motor_task -> cmd_task direction) */
#define NOTIFY_MSG_QUEUE_DEPTH   5U
