/**
 * config_pins.h
 * Pin definitions for CNC Shield v3 + Arduino Uno
 * Supports two common shield variants
 */

#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// ===== SHIELD VARIANT SELECTION =====
// Set to 1 or 2 based on your CNC Shield v3 revision
#define SHIELD_VARIANT 1   // 1: A-axis on D12/D13 (common)
                           // 2: A-axis on A3/A4 (some clones)

// ===== X/Y/Z AXES (Standard across all shields) =====
#define X_STEP_PIN 2
#define X_DIR_PIN  5

#define Y_STEP_PIN 3
#define Y_DIR_PIN  6

#define Z_STEP_PIN 4
#define Z_DIR_PIN  7

// ===== A-AXIS (Variant-dependent) =====
#if SHIELD_VARIANT == 1
  #define A_STEP_PIN 12
  #define A_DIR_PIN  13   // Note: D13 LED will mirror DIR signal (acceptable)
  #define A_MIN_PIN  A0   // Use A0 to avoid conflict with D12 (A_STEP)
#elif SHIELD_VARIANT == 2
  #define A_STEP_PIN A3
  #define A_DIR_PIN  A4
  #define A_MIN_PIN  12   // D12 is free in this variant
#else
  #error "SHIELD_VARIANT must be 1 or 2"
#endif

// ===== DRIVER ENABLE (Shared, Active-LOW) =====
#define DRV_EN_PIN 8      // LOW = enabled, HIGH = disabled

// ===== ENDSTOPS (MIN, Normally-Open to GND with INPUT_PULLUP) =====
#define X_MIN_PIN 9
#define Y_MIN_PIN 10
#define Z_MIN_PIN 11
// A_MIN_PIN defined above based on variant

// ===== HARDWARE E-STOP (Optional, Normally-Closed loop) =====
#define ESTOP_IN_PIN A1   // NC→GND loop; open circuit = E-Stop triggered

// ===== UART =====
// Hardware Serial: RX=D0, TX=D1 (shared with USB; disconnect during upload)
#define UART_BAUD 115200

#endif // CONFIG_PINS_H