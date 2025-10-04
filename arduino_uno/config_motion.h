/**
 * config_motion.h
 * Motor parameters, limits, speeds, and homing configuration
 */

#ifndef CONFIG_MOTION_H
#define CONFIG_MOTION_H

#include <stdint.h>

// ===== MOTOR PARAMETERS =====
// Steps per revolution at 1x microstepping
#define J1_STEPS_PER_REV 200    // NEMA-17 (1.8° = 200 steps/rev)
#define J2_STEPS_PER_REV 200    // NEMA-17
#define J3_STEPS_PER_REV 2048   // 28BYJ-48 bipolar (after 64:1 gearbox)
#define J4_STEPS_PER_REV 2048   // 28BYJ-48 bipolar

// Microstepping multipliers (set via DRV8825 jumpers)
#define J1_MICROSTEP 16   // 1/16 for NEMA joints
#define J2_MICROSTEP 16
#define J3_MICROSTEP 1    // Full-step for 28BYJ (smoother at low current)
#define J4_MICROSTEP 1

// Effective steps per revolution (physical × microstep)
const long STEPS_PER_REV[4] = {
  J1_STEPS_PER_REV * J1_MICROSTEP,  // J1 = 3200
  J2_STEPS_PER_REV * J2_MICROSTEP,  // J2 = 3200
  J3_STEPS_PER_REV * J3_MICROSTEP,  // J3 = 2048
  J4_STEPS_PER_REV * J4_MICROSTEP   // J4 = 2048
};

// ===== VELOCITY & ACCELERATION LIMITS =====
// Maximum speeds (steps/s) - used for mapping normalized velocities
const float J_MAX_SPEED[4] = { 2000.0, 2000.0, 400.0, 400.0 };

// Maximum accelerations (steps/s²)
const float J_MAX_ACCEL[4] = { 1500.0, 1500.0, 300.0, 300.0 };

// Direction inversion (true if motor runs backward)
const bool J_DIR_INVERT[4] = { false, false, false, false };

// ===== SOFT LIMITS (steps from home position) =====
// WARNING: Measure these after mechanical assembly!
// Start conservative; expand after testing.
const long J_MIN_LIMIT[4] = { -20000, -15000, 0, 0 };
const long J_MAX_LIMIT[4] = {  20000,  15000, 4000, 4000 };

// Provisional limits (used when unhomed, if motion allowed)
// Very conservative to prevent crashes
const long J_MIN_PROVISIONAL[4] = { -5000, -5000, 0, 0 };
const long J_MAX_PROVISIONAL[4] = {  5000,  5000, 1000, 1000 };

// ===== HOMING CONFIGURATION =====
// Homing direction: -1 = toward MIN endstop, +1 = toward MAX
const int8_t HOMING_DIR[4] = { -1, -1, -1, -1 };

// Homing speeds (steps/s)
#define HOMING_SPEED_FAST 800   // Initial seek
#define HOMING_SPEED_SLOW 100   // Precision re-seek
#define HOMING_BACKOFF    200   // Steps to back off after trigger

// Homing timeout per joint (ms)
#define HOMING_TIMEOUT_MS 30000  // 30 seconds

// ===== SAFETY TIMEOUTS =====
#define UART_TIMEOUT_MS 300      // Stop if no valid packet for >300ms
#define TELEMETRY_INTERVAL_MS 100 // Send status at 10 Hz

// ===== ENDSTOP BEHAVIOR =====
// If true, hitting an endstop during operation triggers full E-Stop
// If false, only clamps motion in that direction (recommended)
#define ENDSTOP_AS_ESTOP false

#endif // CONFIG_MOTION_H