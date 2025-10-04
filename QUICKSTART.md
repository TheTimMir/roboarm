# RoboArm Quick Start Guide
**Get up and running in 30 minutes**

## What You Need

### Hardware (assembled)
- ✅ Arduino Uno + CNC Shield v3 with 4× DRV8825 drivers
- ✅ ESP-01S with 3.3V LDO regulator
- ✅ 4× Motors (2× NEMA-17, 2× 28BYJ-48 bipolar-converted)
- ✅ 12V 5A power supply
- ✅ 4× Endstop switches wired
- ✅ UART connected (ESP↔Uno)

### Software
- ✅ Arduino IDE or PlatformIO installed
- ✅ Libraries: `AccelStepper`, `ArduinoJson`, `GyverHub`

### Tools
- 🔧 Multimeter
- 🔧 Ceramic screwdriver (for Vref)
- 🔧 Computer with WiFi

---

## Step 1: Configure Firmware (5 min)

### Arduino Uno

**1.1 Open** `arduino_uno/config_pins.h`

**1.2 Check your shield's A-axis pins:**
   - Look at the shield: Is pin 13 labeled "A-DIR"? → **Variant 1**
   - Or is it on an analog pin (A3/A4)? → **Variant 2**

**1.3 Set the variant:**
```cpp
#define SHIELD_VARIANT 1   // Change to 2 if needed
```

**1.4 Verify motor directions** (you'll test these later):
```cpp
// In config_motion.h - leave as false for now
const bool J_DIR_INVERT[4] = { false, false, false, false };
```

### ESP-01S

**2.1 Open** `esp01s/src/config.h`

**2.2 Enter your WiFi credentials:**
```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourPassword123"
```

**2.3 Adjust packet rate if needed:**
```cpp
#define PACKET_RATE_HZ 30  // 20-50 Hz, 30 is good default
```

---

## Step 2: Set Driver Currents (10 min)

**⚠️ CRITICAL STEP - Don't skip!**

### 2.1 Disconnect Motors
Remove all motor cables from drivers (to protect motors during adjustment).

### 2.2 Power On System
Apply 12V power. Uno should boot, ESP LED will blink.

### 2.3 Measure Vref on Each Driver

**NEMA-17 Drivers (X, Y channels):**
1. Touch multimeter **black probe to shield GND**
2. Touch **red probe to potentiometer wiper** (tiny metal screw on driver)
3. Read voltage
4. **Target: 0.60-0.70V**
5. Adjust with ceramic screwdriver:
   - Clockwise = increase
   - Counter-clockwise = decrease
6. Repeat for second NEMA driver

**28BYJ Drivers (Z, A channels):**
1. Same procedure
2. **Target: 0.05-0.075V** (start at 0.05V)
3. These need very low current!

### 2.4 Verify Settings
Double-check all four Vrefs. Write them down:
```
X (J1): _____ V (target 0.65V)
Y (J2): _____ V (target 0.65V)
Z (J3): _____ V (target 0.05V)
A (J4): _____ V (target 0.05V)
```

### 2.5 Power Off
Disconnect 12V before reconnecting motors.

---

## Step 3: Upload Firmware (5 min)

### Arduino Uno

**3.1 Disconnect UART** from ESP (or you'll get upload errors)

**3.2 In Arduino IDE:**
- Board: **Arduino Uno**
- Port: Select your Uno's port
- Upload: `arduino_uno/uno_roboarm.ino`

**3.3 Open Serial Monitor** at 115200 baud

**3.4 You should see:**
```
{"info":"Uno RoboArm v1.0"}
{"info":"Init complete. Awaiting HOME command."}
```

✅ If you see this, Uno firmware is working!

### ESP-01S

**4.1 Disconnect from Uno UART** (important!)

**4.2 Using PlatformIO:**
```bash
cd esp01s
pio run -t upload
```

**Or Arduino IDE:**
- Board: **Generic ESP8266 Module**
- Flash Size: **1MB (FS: 64KB OTA)**
- Upload Speed: **115200**
- Upload

**4.3 Watch GPIO2 LED:**
- Blinking fast = Connecting to WiFi
- Solid ON = Connected successfully

**4.4 Find ESP IP Address:**
- Check your router's DHCP client list
- Look for device named "RoboArm" or "ESP_XXXXXX"
- Write down IP: **___.___.___.___**

---

## Step 4: Reconnect & Test (5 min)

### 4.1 Power Off Everything

### 4.2 Reconnect Motors
Connect all four motors to their drivers (verify coil pairs!).

### 4.3 Reconnect UART
```
ESP TX (GPIO1) → Uno RX (D0)
ESP RX (GPIO3) → Uno TX (D1)
GND           → Common GND
```

### 4.4 Add Cooling
Attach heatsinks to X/Y drivers. Position fan if available.

### 4.5 Power On
Apply 12V. Both boards should boot.

---

## Step 5: Web Interface Test (5 min)

### 5.1 Open Browser
On phone or computer (same WiFi network).

Navigate to: **http://YOUR_ESP_IP/**

### 5.2 You Should See:
- Title: "RoboArm Control"
- Two joystick widgets
- DEADMAN switch (OFF)
- Control buttons (Enable, E-Stop, HOME, etc.)
- Status indicators (currently red/yellow)

✅ If UI loads, communication is working!

### 5.3 Quick Button Test
**Don't move joysticks yet!**

1. Click **"Enable"** button
2. Check status LED turns green
3. Click **"Disable"**
4. Status back to yellow

---

## Step 6: First Motion (5 min)

### ⚠️ Safety Check
- Clear workspace (nothing touching robot)
- Motors free to move (no binding)
- Endstops positioned but not yet touching
- E-Stop easily reachable

### 6.1 Manual Single Motor Test

In web UI:
1. Click **"Enable"**
2. **Hold DEADMAN switch** (keep finger on it)
3. **Gently push left joystick forward** (Y+, ~20%)
4. **J2 (shoulder) should rotate slowly**
5. Release joystick → motor stops
6. Release DEADMAN → all motion locked

**If motor doesn't move:**
- Check Vref is >0.5V for NEMA
- Verify driver LED is on
- Check motor wiring (continuity)

**If motor runs backward:**
- Note which joint
- Power off
- Edit `config_motion.h`: `J_DIR_INVERT[1] = true` (for J2)
- Re-upload Uno firmware
- Test again

### 6.2 Test Each Joint

Repeat for:
- **Left X** (J1 base) - should rotate base
- **Right X** (J3 wrist) - should move wrist
- **Right Y** (J4 gripper) - should open/close

**28BYJ motors (J3/J4) move slower - this is normal!**

✅ If all 4 joints respond to joysticks: **Motion system working!**

---

## Step 7: Homing (3 min)

### 7.1 Position Robot
Manually move joints so they're **~5-10cm away from MIN endstops**.

### 7.2 Verify Endstops
Manually trigger each switch (short to GND). Check that position display shows limit flag.

### 7.3 Run Homing

In web UI:
1. Click **"Disable"** (if enabled)
2. Click **"HOME All"** button
3. **Watch carefully!** Be ready to click E-Stop if anything goes wrong

**Expected behavior:**
- J1 moves toward MIN endstop, stops, backs off, re-seeks
- J2 does same
- J3 does same
- J4 does same
- After 30-60 seconds: All "Homed" indicators turn green
- Positions reset to [0, 0, 0, 0]

**If homing fails:**
- Check endstop wiring (pullup? NO→GND?)
- Verify `HOMING_DIR` in `config_motion.h` is correct direction
- Increase timeout if joints are slow

✅ **Homing complete? You're fully operational!**

---

## Step 8: Test Safety Systems (2 min)

### 8.1 E-Stop Test
1. Enable drivers, hold deadman, move joystick
2. Click **"E-STOP"** button
3. Motion should **stop immediately**
4. Try to move → nothing happens (good!)
5. Click **"Clear E-Stop"**
6. Click **"Enable"**
7. Motion works again

### 8.2 Deadman Test
1. Move joystick while holding DEADMAN
2. Release DEADMAN (lift finger)
3. Motion stops immediately
4. Press DEADMAN again
5. Motion resumes

### 8.3 Timeout Test
1. Start motion
2. **Close browser tab** (or turn off WiFi on phone)
3. After ~300ms, motors should stop
4. Reconnect to UI
5. System still responsive

✅ **All safety systems working? You're done!**

---

## You're Ready to Use RoboArm! 🎉

### Normal Operation Flow:
1. Power on system
2. Open web UI
3. Click **"HOME All"** (once per power-on)
4. Click **"Enable"**
5. **Hold DEADMAN**
6. Control with joysticks
7. Release to stop

### Tips for Smooth Operation:
- Start with small joystick movements (20-30%)
- Don't slam joysticks to full (causes jerky motion)
- Watch position limits in UI
- Release DEADMAN when positioning manually
- Home after any mechanical adjustment

---

## Troubleshooting Quick Fixes

| Problem | Quick Fix |
|---------|-----------|
| Motor stutters | Increase Vref by 0.05V |
| Motor runs backward | Set `J_DIR_INVERT[i] = true` |
| Web UI won't load | Check ESP IP, verify WiFi connection |
| Homing fails | Check endstop wiring, verify `HOMING_DIR` |
| UART timeout constantly | Check TX↔RX crossover, close Serial Monitor |
| Driver very hot | Add heatsink/fan, reduce Vref slightly |
| 28BYJ rough motion | Lower Vref to 0.05V, use full-step mode |

---

## Next Steps

**Now that it's working:**
1. **Tune motion** - Adjust speeds/accels in `config_motion.h`
2. **Measure limits** - Find actual min/max positions, update soft limits
3. **Add features** - Implement preset positions, macros
4. **Build kinematics** - Add inverse kinematics for Cartesian control

**See full documentation:**
- [README.md](README.md) - Complete project overview
- [TESTING_GUIDE.md](docs/TESTING_GUIDE.md) - Comprehensive testing
- [WIRING_DIAGRAM.md](docs/WIRING_DIAGRAM.md) - Detailed connections

---

## Success Checklist

- [ ] All 4 motors respond to joysticks
- [ ] Directions correct (or inverted in firmware)
- [ ] Homing completes successfully
- [ ] E-Stop works (stops immediately)
- [ ] Deadman switch prevents motion when released
- [ ] UART timeout stops motion (~300ms)
- [ ] Web UI responsive (<100ms lag)
- [ ] No overheating (drivers <80°C)
- [ ] No missed steps during motion
- [ ] Endstops trigger correctly

**All checked? Congratulations - your RoboArm is fully operational! 🤖✅**

---

## Safety Reminders

- ⚠️ Always have E-Stop accessible
- ⚠️ Clear workspace before motion
- ⚠️ Don't bypass safety systems
- ⚠️ Monitor thermal conditions
- ⚠️ Home after any mechanical change

**Have fun building and experimenting safely!**