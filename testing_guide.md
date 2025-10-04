# RoboArm Testing & Validation Guide

## Prerequisites Checklist

### Hardware
- [ ] All 4 motors wired to correct DRV8825 channels
- [ ] 28BYJ-48 converted to bipolar (red wire unused, common trace cut)
- [ ] Coil pairs verified: Blue↔Pink (A), Yellow↔Orange (B)
- [ ] 12V PSU connected to CNC Shield VMOT (5A minimum)
- [ ] ESP-01S powered from dedicated 3.3V LDO (NOT Uno's 3.3V pin)
- [ ] 100µF+ capacitor on VMOT rail
- [ ] Heatsinks + fan on NEMA-17 driver channels
- [ ] Common ground: PSU, Uno, ESP, endstops
- [ ] Endstops wired NO→GND (normally open to ground)
- [ ] UART cross-connected: ESP TX→Uno RX, ESP RX→Uno TX

### Software
- [ ] Arduino IDE or PlatformIO installed
- [ ] Libraries installed:
  - Arduino: `AccelStepper`, `ArduinoJson`
  - ESP: `GyverHub`, `ArduinoJson`
- [ ] WiFi credentials updated in `config.h`
- [ ] Shield variant verified and set in `config_pins.h`

---

## Phase 1: Power & Current Calibration

### 1.1 Verify Power Rails
1. **Without motors connected:**
   - Measure VMOT at shield: Should read **12.0V ±0.3V**
   - Measure Uno 5V pin: Should read **4.8-5.2V**
   - Measure ESP 3.3V: Should read **3.2-3.4V**

2. **Check driver enable:**
   - Power on with Uno code loaded
   - Drivers should be **disabled** on boot (EN pin HIGH)
   - LEDs on DRV8825 boards may be dim/off

### 1.2 Set Vref (Critical!)

**Tools needed:** Multimeter, small ceramic screwdriver

**NEMA-17 Channels (X, Y):**
1. Disconnect motor from driver
2. Power on system
3. Measure voltage between potentiometer wiper and GND
4. Target: **0.60-0.70V** (for Imax = 1.2-1.4A with R100 sense resistors)
5. Adjust potentiometer clockwise to increase, CCW to decrease
6. Repeat for second NEMA channel

**28BYJ Channels (Z, A):**
1. Target: **0.05-0.075V** (for Imax = 0.10-0.15A)
2. Start at **0.05V** (very conservative)
3. Can increase slightly if motion is rough, but stay below 0.08V

**Verification:**
- Re-measure Vref after 5 minutes of operation
- Should remain stable (±0.02V)
- If drifts, suspect poor contact or bad potentiometer

### 1.3 Thermal Check
1. Connect motors, enable drivers via serial command: `{"cmd":"enable","val":1}`
2. Manually move each motor (slow, <50% speed)
3. After 5 minutes:
   - DRV8825 heatsinks: **Warm but touchable** (60-70°C OK)
   - NEMA motors: **Warm** (40-50°C OK)
   - 28BYJ motors: **Barely warm** (30-35°C OK)
4. If too hot: **Reduce Vref by 0.05V** and retest

---

## Phase 2: Arduino Uno Basic Tests

### 2.1 Serial Monitor Test
1. Upload Uno firmware
2. Open Serial Monitor at **115200 baud**
3. Should see:
   ```
   {"info":"Uno RoboArm v1.0"}
   {"info":"Init complete. Awaiting HOME command."}
   ```
4. Send test command: `{"cmd":"enable","val":1}`
5. Should see: `{"info":"Drivers enabled"}`
6. Send: `{"cmd":"enable","val":0}`
7. Should see: `{"info":"Drivers disabled"}`

### 2.2 Manual Velocity Test (No ESP)
1. Enable drivers: `{"cmd":"enable","val":1}`
2. Send velocity command:
   ```json
   {"t":1000,"mode":"vel","q":[0.2,0,0,0],"en":1,"seq":1}
   ```
3. **Expected:** J1 (base) rotates slowly
4. Send: `{"t":2000,"mode":"vel","q":[0,0,0,0],"en":1,"seq":2}`
5. **Expected:** J1 stops smoothly

6. Test each joint individually:
   - J2: `"q":[0,0.2,0,0]`
   - J3: `"q":[0,0,0.2,0]`
   - J4: `"q":[0,0,0,0.2]`

**Troubleshooting:**
- Motor runs backward: Set `J_DIR_INVERT[i] = true` in `config_motion.h`
- Motor stutters: Check Vref, reduce speed to 0.1
- No movement: Check STEP/DIR pin definitions, verify EN is LOW

### 2.3 Timeout Test
1. Enable drivers and send velocity packet
2. **Stop sending packets**
3. After **300ms**: Motors should stop, message: `{"warn":"UART timeout"}`

### 2.4 Deadman Test
1. Send: `{"t":1000,"mode":"vel","q":[0.3,0,0,0],"en":1,"seq":1}`
2. Motor moves
3. Send: `{"t":1100,"mode":"vel","q":[0.3,0,0,0],"en":0,"seq":2}`
4. **Expected:** Immediate stop

---

## Phase 3: Homing Routine

### 3.1 Endstop Verification
1. **Manually trigger each endstop** (short to GND)
2. Send via Serial Monitor:
   ```
   Check X_MIN: Touch wire to D9 and GND
   Check Y_MIN: Touch wire to D10 and GND
   Check Z_MIN: Touch wire to D11 and GND
   Check A_MIN: Touch wire to A0 (or D12) and GND
   ```
3. Telemetry `"lim"` field should show 1 when triggered

### 3.2 First Homing Test (Single Joint)
**Safety:** Have E-Stop ready, motors should be far from mechanical limits

1. Disable drivers if enabled: `{"cmd":"enable","val":0}`
2. Position J1 ~10cm from MIN endstop
3. Send: `{"cmd":"home"}`
4. **Expected sequence:**
   - Drivers enable
   - J1 moves toward MIN at 800 steps/s
   - Endstop triggers → stops
   - Backs off 200 steps
   - Re-seeks slowly at 100 steps/s
   - Sets position to 0
   - All joints repeat process
   - Drivers disable
   - Message: `{"info":"Homing complete"}`

**Troubleshooting:**
- Timeout: Increase `HOMING_TIMEOUT_MS`, check endstop wiring
- Crashes into limit: Verify `HOMING_DIR` is correct for that joint
- Doesn't stop at endstop: Check pullup resistor, verify NO→GND wiring

### 3.3 Full Homing Test
1. Position all joints mid-range
2. Send: `{"cmd":"home"}`
3. All joints should home sequentially
4. Verify telemetry shows `"homed":[1,1,1,1]`

---

## Phase 4: ESP-01S Web UI

### 4.1 WiFi Connection
1. Upload ESP firmware (disconnect from Uno UART first!)
2. Power ESP from 3.3V LDO
3. Watch GPIO2 LED:
   - **Blinking:** Connecting to WiFi
   - **Solid ON:** Connected
4. Check router DHCP table for device IP (or use serial monitor if debug enabled)

### 4.2 Access Web UI
1. Open browser (phone or PC on same network)
2. Navigate to: `http://<ESP_IP>/`
3. GyverHub interface should load:
   - Title: "RoboArm Control"
   - Two joysticks
   - Deadman switch
   - Control buttons
   - Status indicators

### 4.3 UI Functionality Test (No Motion)
1. **Deadman button:**
   - Press: Switch turns ON
   - Release: Switch turns OFF
2. **Joysticks:**
   - Move left joystick: Should be responsive
   - Move right joystick: Should be responsive
3. **Buttons:**
   - Click "Enable": Should send command to Uno
   - Check Uno serial: `{"info":"Drivers enabled"}`

---

## Phase 5: Integrated System Test

### 5.1 Reconnect UART
1. Power off both devices
2. Connect ESP TX (GPIO1) → Uno RX (D0)
3. Connect ESP RX (GPIO3) → Uno TX (D1)
4. Verify common ground
5. **Important:** Cannot use Serial Monitor on Uno during operation!

### 5.2 First Motion Test
1. Power on system
2. Click "HOME All" button on web UI
3. Wait for homing to complete (~30-60 seconds)
4. Click "Enable" button
5. **Hold Deadman button**
6. Gently move left joystick forward (Y+)
7. **Expected:** J2 (shoulder) moves smoothly
8. Release joystick → motor stops within 200ms
9. Release deadman → all motion inhibited

### 5.3 Full Range Test
1. With deadman held, test each axis:
   - Left X: J1 base rotation
   - Left Y: J2 shoulder
   - Right X: J3 wrist
   - Right Y: J4 gripper
2. Check for smooth acceleration/deceleration
3. Verify no stalls or missed steps

### 5.4 Soft Limit Test
1. Move J1 to near `J1_MAX_LIMIT` (watch position in UI)
2. Try to move further: Motion should clamp
3. Move opposite direction: Should work immediately
4. Repeat for all joints

---

## Phase 6: Safety Systems

### 6.1 E-Stop Button Test
1. Start motion (deadman held, joystick active)
2. Click "E-STOP" button
3. **Expected:**
   - Immediate motion stop
   - Drivers disabled
   - Red "E-STOP ACTIVE" LED
   - Cannot re-enable until cleared
4. Click "Clear E-Stop"
5. Click "Enable"
6. Motion should resume normally

### 6.2 WiFi Disconnect Test
1. Start motion
2. **Turn off WiFi on phone/laptop** (or close browser tab)
3. **Expected within 300ms:**
   - Uno detects UART timeout
   - Motion stops
   - Drivers remain enabled but idle
4. Reconnect to UI
5. Motion resumes when deadman held

### 6.3 Hardware E-Stop Test (if wired)
1. Start motion
2. **Trigger hardware E-Stop** (open NC loop on A1)
3. **Expected:**
   - Immediate stop
   - Message: `{"error":"Hardware E-Stop triggered"}`
   - Requires physical reset + Clear command

### 6.4 Endstop During Operation
1. Enable drivers, home if needed
2. Drive J1 slowly toward MIN endstop
3. When triggered:
   - Motion in that direction stops
   - Can immediately move away from endstop
   - No global E-Stop (unless `ENDSTOP_AS_ESTOP` enabled)

---

## Phase 7: Load & Endurance

### 7.1 10-Minute Continuous Operation
1. Home all joints
2. Enable drivers
3. Run scripted sweep pattern (or manual):
   - Hold deadman
   - Slowly sweep joysticks through full range
   - All 4 joints moving
4. Monitor for 10 minutes:
   - No thermal shutdown
   - No missed steps
   - Positions remain accurate
   - UI remains responsive

### 7.2 Thermal Measurement
After 10-minute test:
- **NEMA drivers:** Should be hot (70-80°C) but stable
- **28BYJ drivers:** Warm (50-60°C)
- **Motors:** All warm but not burning to touch
- **PSU:** Barely warm
- **ESP/Uno:** Room temp

**If any component exceeds safe temp:** Improve cooling or reduce Vref

### 7.3 Position Accuracy
1. Move J1 to position +5000 steps (from telemetry)
2. Send to -5000 steps
3. Return to 0
4. **Expected:** Position should be ±10 steps of 0
5. If drift >50 steps: Check for:
   - Mechanical binding
   - Insufficient Vref (stalling)
   - Loose couplings

---

## Phase 8: Acceptance Criteria

### ✅ Hardware
- [ ] All 4 motors move smoothly in both directions
- [ ] Vref stable after 10-minute test
- [ ] No thermal issues (all components <85°C)
- [ ] Endstops trigger reliably
- [ ] E-Stop (SW & HW) functions correctly

### ✅ Firmware
- [ ] Homing completes successfully
- [ ] Soft limits enforce boundaries
- [ ] UART timeout stops motion within 300ms
- [ ] Deadman release stops immediately
- [ ] Telemetry updates at 10 Hz
- [ ] Web UI responsive (<100ms lag)

### ✅ Integration
- [ ] WiFi disconnect triggers graceful timeout
- [ ] E-Stop recoverable via Clear command
- [ ] 10-minute operation without faults
- [ ] Position accuracy ±10 steps after full sweep
- [ ] No dropped packets (check seq_ack)

---

## Troubleshooting Common Issues

### Motor Stutters/Stalls
- **Vref too low:** Increase by 0.05V
- **Mechanical binding:** Check for obstructions
- **Speed too high:** Reduce `J_MAX_SPEED[i]`
- **Bad coil connection:** Check continuity

### ESP Won't Connect to WiFi
- **Wrong credentials:** Verify SSID/password in `config.h`
- **Power issue:** Measure 3.3V rail under load
- **GPIO0 floating:** Should not be connected during normal operation

### Uno Not Receiving Commands
- **UART swap:** Verify TX→RX, RX→TX crossover
- **Baud mismatch:** Check both set to 115200
- **Serial Monitor open:** Close monitor, it blocks UART

### Homing Never Completes
- **Endstop not wired:** Check continuity
- **Wrong direction:** Flip `HOMING_DIR[i]`
- **Timeout too short:** Increase `HOMING_TIMEOUT_MS`

### UI Joystick Lag
- **WiFi congestion:** Move closer to router
- **Packet rate too high:** Reduce `PACKET_RATE_HZ` to 20
- **ESP heap exhausted:** Check free heap, simplify UI

---

## Next Steps After Acceptance

1. **Tune motion profiles:** Adjust accel/decel for smoother operation
2. **Measure soft limits:** Record actual mechanical limits
3. **Implement macro sequences:** Add preset positions in UI
4. **Add coordinate transforms:** Joint → Cartesian kinematics
5. **Enable trajectory planning:** Smooth multi-point paths

---

## Emergency Procedures

### Runaway Motor
1. **Press E-Stop button** (web UI)
2. If unresponsive: **Cut 12V power** (PSU switch)
3. If still running: **Pull DRV8825 from socket**

### Overheating
1. **Disable drivers immediately**
2. Let cool for 10 minutes
3. Reduce Vref by 0.1V
4. Add cooling (fans, heatsinks)

### Lost Position After Crash
1. **Disable drivers**
2. Manually move joints to safe position
3. **Run HOME command**
4. Verify limits before resuming

---

**Testing Complete!** Your RoboArm should now be fully operational. 🤖