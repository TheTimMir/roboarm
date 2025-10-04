/**
 * ESP-01S RoboArm Web Controller
 * 
 * Hosts GyverHub web UI with dual joysticks + deadman + E-Stop
 * Sends velocity commands to Arduino Uno via UART
 * 
 * Hardware: ESP-01S (ESP8266)
 * Libraries: GyverHub (install via Library Manager)
 */

#include <ESP8266WiFi.h>
#include <GyverHub.h>
#include "config.h"

// ===== GyverHub Setup =====
GyverHub hub("MyDevices", "RoboArm", "");  // prefix, name, icon

// ===== Global State =====
struct JoystickState {
  float x = 0.0;
  float y = 0.0;
  float x_smooth = 0.0;
  float y_smooth = 0.0;
};

JoystickState joyLeft;   // Controls J1 (x) and J2 (y)
JoystickState joyRight;  // Controls J3 (x) and J4 (y)

bool deadmanPressed = false;
bool eStopActive = false;
bool driversEnabled = false;

unsigned long lastPacketTime = 0;
unsigned long lastUiActivityTime = 0;
uint16_t packetSeq = 0;

// Telemetry from Uno
struct RobotStatus {
  long pos[4] = {0, 0, 0, 0};
  bool homed[4] = {false, false, false, false};
  bool en = false;
  bool fault = false;
  unsigned long lastUpdate = 0;
} robotStatus;

// ===== Function Prototypes =====
void buildUI();
void sendVelocityPacket();
void processUnoTelemetry();
float applyDeadzone(float val);
float smoothEMA(float current, float target, float alpha);

// ===== Setup =====
void setup() {
  // Configure status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);  // OFF (active LOW)
  
  // Initialize UART to Uno
  Serial.begin(UART_BAUD);
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Blink LED while connecting
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    delay(200);
  }
  digitalWrite(STATUS_LED_PIN, LOW);  // ON when connected
  
  // Start GyverHub
  hub.onBuild(buildUI);
  hub.begin();
  
  lastUiActivityTime = millis();
}

// ===== Main Loop =====
void loop() {
  hub.tick();
  
  unsigned long now = millis();
  
  // Check for Uno telemetry
  if (Serial.available()) {
    processUnoTelemetry();
  }
  
  // Watchdog: if no UI activity, send en=0
  if (!deadmanPressed && (now - lastUiActivityTime > WATCHDOG_TIMEOUT_MS)) {
    deadmanPressed = false;
  }
  
  // Send velocity packets at configured rate
  if (now - lastPacketTime >= PACKET_INTERVAL_MS) {
    sendVelocityPacket();
    lastPacketTime = now;
  }
  
  // Update UI indicators (GyverHub auto-polls)
  // Status updates handled in buildUI callbacks
}

// ===== GyverHub UI =====
void buildUI() {
  hub.BeginUI();
  
  // Title
  hub.Title_("RoboArm Control");
  
  // Status indicators
  hub.BeginWidgets();
  {
    if (eStopActive) {
      hub.LED_("led_estop", true).label("E-STOP ACTIVE").color(gh::Colors::Red);
    } else if (!robotStatus.en) {
      hub.LED_("led_disabled", true).label("Disabled").color(gh::Colors::Yellow);
    } else {
      hub.LED_("led_ok", true).label("Enabled").color(gh::Colors::Green);
    }
    
    // Homing status
    String homedStr = "Homed: ";
    for (int i = 0; i < 4; i++) {
      homedStr += robotStatus.homed[i] ? "✓" : "✗";
      if (i < 3) homedStr += " ";
    }
    hub.Label_("lbl_homed", homedStr);
  }
  hub.EndWidgets();
  
  // Control section
  hub.Separator();
  hub.Label_("lbl_control", "Hold DEADMAN to enable motion");
  
  // Deadman button (switch that must be held)
  if (hub.Switch_("sw_deadman", &deadmanPressed).label("DEADMAN (Hold)").click()) {
    lastUiActivityTime = millis();
  }
  
  // Joysticks (two separate joysticks side-by-side)
  hub.BeginWidgets();
  {
    // Left joystick (J1/J2)
    if (hub.Joystick_("joy_left", &joyLeft.x, &joyLeft.y).label("Base/Shoulder").click()) {
      lastUiActivityTime = millis();
      
      // Apply smoothing
      joyLeft.x_smooth = smoothEMA(joyLeft.x_smooth, applyDeadzone(joyLeft.x), JOYSTICK_EMA_ALPHA);
      joyLeft.y_smooth = smoothEMA(joyLeft.y_smooth, applyDeadzone(joyLeft.y), JOYSTICK_EMA_ALPHA);
    }
    
    // Right joystick (J3/J4)
    if (hub.Joystick_("joy_right", &joyRight.x, &joyRight.y).label("Wrist/Gripper").click()) {
      lastUiActivityTime = millis();
      
      joyRight.x_smooth = smoothEMA(joyRight.x_smooth, applyDeadzone(joyRight.x), JOYSTICK_EMA_ALPHA);
      joyRight.y_smooth = smoothEMA(joyRight.y_smooth, applyDeadzone(joyRight.y), JOYSTICK_EMA_ALPHA);
    }
  }
  hub.EndWidgets();
  
  // Control buttons
  hub.Separator();
  hub.BeginWidgets();
  {
    // E-Stop button
    if (hub.Button_("btn_estop").label("E-STOP").color(gh::Colors::Red).click()) {
      sendCommand("{\"cmd\":\"estop\"}");
      eStopActive = true;
      lastUiActivityTime = millis();
    }
    
    // Clear E-Stop
    if (hub.Button_("btn_clear").label("Clear E-Stop").color(gh::Colors::Orange).click()) {
      sendCommand("{\"cmd\":\"clear_estop\"}");
      eStopActive = false;
      lastUiActivityTime = millis();
    }
    
    // Enable/Disable drivers
    if (hub.Button_("btn_enable").label(driversEnabled ? "Disable" : "Enable").click()) {
      driversEnabled = !driversEnabled;
      String cmd = "{\"cmd\":\"enable\",\"val\":";
      cmd += driversEnabled ? "1}" : "0}";
      sendCommand(cmd);
      lastUiActivityTime = millis();
    }
    
    // Home button
    if (hub.Button_("btn_home").label("HOME All").color(gh::Colors::Blue).click()) {
      sendCommand("{\"cmd\":\"home\"}");
      lastUiActivityTime = millis();
    }
  }
  hub.EndWidgets();
  
  // Position display
  hub.Separator();
  hub.Label_("lbl_pos_title", "Joint Positions (steps):");
  for (int i = 0; i < 4; i++) {
    String label = "J" + String(i + 1) + ": " + String(robotStatus.pos[i]);
    hub.Label_("lbl_pos_" + String(i), label);
  }
  
  // Connection info
  hub.Separator();
  String ipStr = "IP: " + WiFi.localIP().toString();
  hub.Label_("lbl_ip", ipStr);
  
  unsigned long lastRx = millis() - robotStatus.lastUpdate;
  String statusStr = "Uno: ";
  statusStr += (lastRx < 500) ? "Connected" : "Timeout";
  hub.Label_("lbl_uno_status", statusStr);
  
  hub.EndUI();
}

// ===== UART Communication =====
void sendVelocityPacket() {
  // Construct velocity array [J1, J2, J3, J4]
  // J1 = left.x, J2 = left.y, J3 = right.x, J4 = right.y
  float q[4] = {
    joyLeft.x_smooth,
    joyLeft.y_smooth,
    joyRight.x_smooth,
    joyRight.y_smooth
  };
  
  // Build JSON packet
  String packet = "{\"t\":";
  packet += millis();
  packet += ",\"mode\":\"vel\",\"q\":[";
  for (int i = 0; i < 4; i++) {
    packet += String(q[i], 3);  // 3 decimal places
    if (i < 3) packet += ",";
  }
  packet += "],\"en\":";
  packet += (deadmanPressed && !eStopActive) ? "1" : "0";
  packet += ",\"seq\":";
  packet += packetSeq++;
  packet += "}\n";
  
  Serial.print(packet);
}

void sendCommand(String cmd) {
  Serial.println(cmd);
}

void processUnoTelemetry() {
  String line = Serial.readStringUntil('\n');
  if (line.length() == 0) return;
  
  // Simple JSON parsing (lightweight for ESP-01S)
  // Format: {"t":123,"pos":[...],"homed":[...],"en":1,"fault":0,...}
  
  // Extract timestamp
  int tIdx = line.indexOf("\"t\":");
  if (tIdx >= 0) {
    robotStatus.lastUpdate = millis();
  }
  
  // Extract positions (simplified parsing)
  int posIdx = line.indexOf("\"pos\":[");
  if (posIdx >= 0) {
    int startIdx = posIdx + 7;
    int endIdx = line.indexOf("]", startIdx);
    String posStr = line.substring(startIdx, endIdx);
    
    int lastComma = -1;
    for (int i = 0; i < 4; i++) {
      int nextComma = posStr.indexOf(",", lastComma + 1);
      if (nextComma < 0) nextComma = posStr.length();
      String valStr = posStr.substring(lastComma + 1, nextComma);
      robotStatus.pos[i] = valStr.toInt();
      lastComma = nextComma;
    }
  }
  
  // Extract homed flags
  int homedIdx = line.indexOf("\"homed\":[");
  if (homedIdx >= 0) {
    int startIdx = homedIdx + 9;
    int endIdx = line.indexOf("]", startIdx);
    String homedStr = line.substring(startIdx, endIdx);
    
    int lastComma = -1;
    for (int i = 0; i < 4; i++) {
      int nextComma = homedStr.indexOf(",", lastComma + 1);
      if (nextComma < 0) nextComma = homedStr.length();
      String valStr = homedStr.substring(lastComma + 1, nextComma);
      robotStatus.homed[i] = (valStr.toInt() == 1);
      lastComma = nextComma;
    }
  }
  
  // Extract enable status
  int enIdx = line.indexOf("\"en\":");
  if (enIdx >= 0) {
    int valStart = enIdx + 5;
    robotStatus.en = (line.charAt(valStart) == '1');
  }
  
  // Extract fault status
  int faultIdx = line.indexOf("\"fault\":");
  if (faultIdx >= 0) {
    int valStart = faultIdx + 8;
    robotStatus.fault = (line.charAt(valStart) != '0');
  }
}

// ===== Helper Functions =====
float applyDeadzone(float val) {
  if (abs(val) < JOYSTICK_DEADZONE) {
    return 0.0;
  }
  // Scale remaining range to [-1, 1]
  float sign = (val > 0) ? 1.0 : -1.0;
  float scaled = (abs(val) - JOYSTICK_DEADZONE) / (1.0 - JOYSTICK_DEADZONE);
  return sign * constrain(scaled, 0.0, 1.0);
}

float smoothEMA(float current, float target, float alpha) {
  return current * (1.0 - alpha) + target * alpha;
}