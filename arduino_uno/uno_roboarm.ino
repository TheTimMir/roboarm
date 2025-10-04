/**
 * uno_roboarm.ino
 * Main Arduino Uno sketch for 4-axis robo-arm control
 * 
 * Hardware: CNC Shield v3 + 4× DRV8825 drivers
 * Communication: UART (115200) with ESP-01S
 */

#include <AccelStepper.h>
#include <ArduinoJson.h>
#include "config_pins.h"
#include "config_motion.h"

// ===== GLOBAL STATE =====
AccelStepper stepper[4] = {
  AccelStepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN),
  AccelStepper(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN),
  AccelStepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN),
  AccelStepper(AccelStepper::DRIVER, A_STEP_PIN, A_DIR_PIN)
};

// Joint state
bool jointHomed[4] = { false, false, false, false };
bool driversEnabled = false;
bool eStopActive = false;
bool eStopCleared = true;  // false after E-Stop until cleared

// Communication
unsigned long lastPacketTime = 0;
uint16_t lastSeq = 0;
bool seqInitialized = false;

// Telemetry
unsigned long lastTelemetryTime = 0;
uint16_t telemetrySeqAck = 0;
String currentMode = "vel";

// Endstop pins array
const uint8_t endstopPins[4] = { X_MIN_PIN, Y_MIN_PIN, Z_MIN_PIN, A_MIN_PIN };

// UART buffer
String uartBuffer = "";

// ===== SETUP =====
void setup() {
  // Initialize UART
  Serial.begin(UART_BAUD);
  while (!Serial && millis() < 2000); // Wait up to 2s for serial
  
  Serial.println(F("{\"info\":\"Uno RoboArm v1.0\"}"));
  
  // Configure driver enable pin (active-LOW)
  pinMode(DRV_EN_PIN, OUTPUT);
  digitalWrite(DRV_EN_PIN, HIGH);  // Start disabled
  driversEnabled = false;
  
  // Configure endstops (INPUT_PULLUP, NO→GND)
  for (int i = 0; i < 4; i++) {
    pinMode(endstopPins[i], INPUT_PULLUP);
  }
  
  // Configure hardware E-Stop (optional, NC loop)
  pinMode(ESTOP_IN_PIN, INPUT_PULLUP);
  
  // Initialize steppers
  for (int i = 0; i < 4; i++) {
    stepper[i].setMaxSpeed(J_MAX_SPEED[i]);
    stepper[i].setAcceleration(J_MAX_ACCEL[i]);
    stepper[i].setCurrentPosition(0);
    
    // Apply direction inversion
    if (J_DIR_INVERT[i]) {
      stepper[i].setPinsInverted(true, false, false);
    }
  }
  
  Serial.println(F("{\"info\":\"Init complete. Awaiting HOME command.\"}"));
}

// ===== MAIN LOOP =====
void loop() {
  unsigned long now = millis();
  
  // 1. Check hardware E-Stop
  checkHardwareEStop();
  
  // 2. Process UART commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCommand(uartBuffer);
      uartBuffer = "";
    } else if (c != '\r') {
      uartBuffer += c;
      if (uartBuffer.length() > 200) {  // Prevent buffer overflow
        uartBuffer = "";
      }
    }
  }
  
  // 3. Check UART timeout
  if (driversEnabled && (now - lastPacketTime > UART_TIMEOUT_MS)) {
    stopAllMotion();
    Serial.println(F("{\"warn\":\"UART timeout\"}"));
  }
  
  // 4. Run stepper control
  if (driversEnabled && !eStopActive) {
    for (int i = 0; i < 4; i++) {
      stepper[i].run();
    }
    checkEndstops();
  }
  
  // 5. Send telemetry
  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    sendTelemetry();
    lastTelemetryTime = now;
  }
}

// ===== COMMAND PROCESSING =====
void processCommand(String& json) {
  if (json.length() == 0) return;
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print(F("{\"error\":\"JSON parse failed: "));
    Serial.print(error.c_str());
    Serial.println(F("\"}"));
    return;
  }
  
  lastPacketTime = millis();
  
  // Handle special commands
  if (doc.containsKey("cmd")) {
    String cmd = doc["cmd"].as<String>();
    
    if (cmd == "home") {
      handleHomeCommand(doc);
    } else if (cmd == "estop") {
      handleEStopCommand();
    } else if (cmd == "clear_estop") {
      handleClearEStopCommand();
    } else if (cmd == "enable") {
      handleEnableCommand(doc);
    }
    return;
  }
  
  // Handle velocity control packet
  if (doc.containsKey("mode") && doc["mode"] == "vel") {
    handleVelocityCommand(doc);
  }
}

void handleVelocityCommand(JsonDocument& doc) {
  // Validate sequence
  if (doc.containsKey("seq")) {
    uint16_t seq = doc["seq"];
    if (seqInitialized && seq == lastSeq) {
      return;  // Duplicate packet
    }
    lastSeq = seq;
    seqInitialized = true;
    telemetrySeqAck = seq;
  }
  
  // Check enable flag
  int en = doc["en"] | 0;
  if (en == 0) {
    stopAllMotion();
    return;
  }
  
  // Safety checks
  if (eStopActive || !driversEnabled) {
    return;
  }
  
  // Extract velocities
  JsonArray q = doc["q"];
  if (q.size() != 4) return;
  
  for (int i = 0; i < 4; i++) {
    float normVel = q[i];
    normVel = constrain(normVel, -1.0, 1.0);
    
    // Map to steps/s
    float targetSpeed = normVel * J_MAX_SPEED[i];
    
    // Apply soft limits (clamp velocity if at limit)
    long currentPos = stepper[i].currentPosition();
    bool atMin = (currentPos <= getMinLimit(i));
    bool atMax = (currentPos >= getMaxLimit(i));
    
    if ((atMin && targetSpeed < 0) || (atMax && targetSpeed > 0)) {
      targetSpeed = 0;  // Clamp at limit
    }
    
    stepper[i].setSpeed(targetSpeed);
  }
  
  currentMode = "vel";
}

void handleHomeCommand(JsonDocument& doc) {
  if (driversEnabled) {
    Serial.println(F("{\"error\":\"Disable drivers before homing\"}"));
    return;
  }
  
  Serial.println(F("{\"info\":\"Homing started\"}"));
  
  // Enable drivers temporarily
  digitalWrite(DRV_EN_PIN, LOW);
  delay(50);  // Let drivers stabilize
  
  // Home all joints
  for (int i = 0; i < 4; i++) {
    if (!homeJoint(i)) {
      Serial.print(F("{\"error\":\"Homing failed on J"));
      Serial.print(i);
      Serial.println(F("\"}"));
      digitalWrite(DRV_EN_PIN, HIGH);
      return;
    }
  }
  
  // Mark all as homed
  for (int i = 0; i < 4; i++) {
    jointHomed[i] = true;
  }
  
  digitalWrite(DRV_EN_PIN, HIGH);  // Disable after homing
  Serial.println(F("{\"info\":\"Homing complete\"}"));
}

bool homeJoint(int joint) {
  int dir = HOMING_DIR[joint];
  uint8_t endstopPin = endstopPins[joint];
  unsigned long startTime = millis();
  
  // Phase 1: Fast seek
  stepper[joint].setSpeed(dir * HOMING_SPEED_FAST);
  while (digitalRead(endstopPin) == HIGH) {  // HIGH = not triggered (pullup)
    stepper[joint].runSpeed();
    if (millis() - startTime > HOMING_TIMEOUT_MS) {
      return false;  // Timeout
    }
  }
  
  // Phase 2: Back off
  stepper[joint].move(-dir * HOMING_BACKOFF);
  while (stepper[joint].distanceToGo() != 0) {
    stepper[joint].run();
  }
  delay(100);
  
  // Phase 3: Slow re-seek
  stepper[joint].setSpeed(dir * HOMING_SPEED_SLOW);
  while (digitalRead(endstopPin) == HIGH) {
    stepper[joint].runSpeed();
  }
  
  // Set zero position
  stepper[joint].setCurrentPosition(0);
  stepper[joint].setSpeed(0);
  
  return true;
}

void handleEStopCommand() {
  eStopActive = true;
  eStopCleared = false;
  stopAllMotion();
  disableDrivers();
  Serial.println(F("{\"info\":\"E-Stop activated\"}"));
}

void handleClearEStopCommand() {
  if (eStopActive) {
    eStopActive = false;
    eStopCleared = true;
    Serial.println(F("{\"info\":\"E-Stop cleared. Use enable command.\"}"));
  }
}

void handleEnableCommand(JsonDocument& doc) {
  int val = doc["val"] | 0;
  
  if (val == 1) {
    if (eStopActive) {
      Serial.println(F("{\"error\":\"Clear E-Stop first\"}"));
      return;
    }
    enableDrivers();
  } else {
    disableDrivers();
  }
}

// ===== MOTION CONTROL =====
void stopAllMotion() {
  for (int i = 0; i < 4; i++) {
    stepper[i].setSpeed(0);
    stepper[i].stop();  // Sets target to current position
  }
}

void enableDrivers() {
  digitalWrite(DRV_EN_PIN, LOW);
  driversEnabled = true;
  Serial.println(F("{\"info\":\"Drivers enabled\"}"));
}

void disableDrivers() {
  digitalWrite(DRV_EN_PIN, HIGH);
  driversEnabled = false;
  stopAllMotion();
  Serial.println(F("{\"info\":\"Drivers disabled\"}"));
}

// ===== SAFETY CHECKS =====
void checkHardwareEStop() {
  // NC loop: LOW = E-Stop triggered
  if (digitalRead(ESTOP_IN_PIN) == LOW) {
    if (!eStopActive) {
      eStopActive = true;
      eStopCleared = false;
      disableDrivers();
      Serial.println(F("{\"error\":\"Hardware E-Stop triggered\"}"));
    }
  }
}

void checkEndstops() {
  for (int i = 0; i < 4; i++) {
    if (digitalRead(endstopPins[i]) == LOW) {  // LOW = triggered
      if (ENDSTOP_AS_ESTOP) {
        handleEStopCommand();
        return;
      } else {
        // Just clamp velocity in that direction
        float speed = stepper[i].speed();
        int dir = HOMING_DIR[i];
        if ((dir < 0 && speed < 0) || (dir > 0 && speed > 0)) {
          stepper[i].setSpeed(0);
        }
      }
    }
  }
}

long getMinLimit(int joint) {
  return jointHomed[joint] ? J_MIN_LIMIT[joint] : J_MIN_PROVISIONAL[joint];
}

long getMaxLimit(int joint) {
  return jointHomed[joint] ? J_MAX_LIMIT[joint] : J_MAX_PROVISIONAL[joint];
}

// ===== TELEMETRY =====
void sendTelemetry() {
  StaticJsonDocument<256> doc;
  
  doc["t"] = millis();
  
  JsonArray pos = doc.createNestedArray("pos");
  for (int i = 0; i < 4; i++) {
    pos.add(stepper[i].currentPosition());
  }
  
  JsonArray homed = doc.createNestedArray("homed");
  for (int i = 0; i < 4; i++) {
    homed.add(jointHomed[i] ? 1 : 0);
  }
  
  JsonArray lim = doc.createNestedArray("lim");
  for (int i = 0; i < 4; i++) {
    int limState = 0;
    if (digitalRead(endstopPins[i]) == LOW) limState |= 1;  // At min
    // Could add max endstop check here if wired
    lim.add(limState);
  }
  
  doc["en"] = driversEnabled ? 1 : 0;
  doc["fault"] = eStopActive ? 1 : 0;
  doc["mode"] = currentMode;
  doc["seq_ack"] = telemetrySeqAck;
  
  serializeJson(doc, Serial);
  Serial.println();
}