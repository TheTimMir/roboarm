/**
 * config.h
 * ESP-01S configuration for robo-arm web UI
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===== WiFi Configuration =====
#define WIFI_SSID "YourNetworkSSID"
#define WIFI_PASSWORD "YourPassword"
#define DEVICE_NAME "RoboArm"

// ===== UART Configuration =====
#define UART_BAUD 115200
#define UART_TX_PIN 1  // GPIO1 (TXD)
#define UART_RX_PIN 3  // GPIO3 (RXD)

// ===== Control Parameters =====
#define PACKET_RATE_HZ 30          // Command packet rate (20-50 Hz recommended)
#define PACKET_INTERVAL_MS (1000 / PACKET_RATE_HZ)

#define WATCHDOG_TIMEOUT_MS 150    // Send en=0 if no UI activity

// Joystick parameters
#define JOYSTICK_DEADZONE 0.1      // Ignore inputs below this magnitude
#define JOYSTICK_EMA_ALPHA 0.3     // Smoothing: 0=none, 1=no smoothing

// ===== Pin Configuration =====
// ESP-01S only has GPIO0, GPIO2 available (GPIO1/3 are UART)
// We'll use GPIO2 for status LED (optional)
#define STATUS_LED_PIN 2           // Built-in LED (active LOW)

// ===== Debug =====
// WARNING: Serial debug conflicts with UART to Uno!
// Only enable for WiFi testing without Uno connected
#define ENABLE_SERIAL_DEBUG false

#endif // CONFIG_H