# RoboArm 4-Axis Controller

**Web-controlled robotic arm with ESP-01S + Arduino Uno + CNC Shield v3**

## Overview

This project implements a complete control system for a 4-axis robotic arm using:
- **ESP-01S (ESP8266)**: Web UI via GyverHub, sends velocity commands over UART
- **Arduino Uno**: Authoritative motion controller, AccelStepper-based, handles safety
- **CNC Shield v3**: Mounts 4× DRV8825 stepper drivers
- **Motors**: 2× NEMA-17 (high torque), 2× 28BYJ-48 bipolar (wrist/gripper)

### Key Features
✅ Dual-joystick web interface (accessible from phone/PC)  
✅ Deadman safety switch (motion only while held)  
✅ Software & hardware E-Stop  
✅ Automatic homing with endstops  
✅ Soft limit enforcement  
✅ UART timeout protection  
✅ Real-time telemetry (position, status)  

---

## Hardware Requirements

### Core Components
| Component | Quantity | Notes |
|-----------|----------|-------|
| Arduino Uno | 1 | ATmega328P |
| CNC Shield v3 | 1 | Check A-axis variant |
| DRV8825 Drivers | 4 | With heatsinks |
| ESP-01S (ESP8266) | 1 | 1MB flash minimum |
| NEMA-17 (JK42HS40-1704) | 2 | 1.7A rated, base/shoulder |
| 28BYJ-48 (5V, bipolar mod) | 2 | Wrist/gripper axes |
| 12V PSU | 1 | 5A minimum (60W+) |
| 3.3V LDO Regulator | 1 | AMS1117-3.3 or better, 500mA |

### Additional Parts
- **Endstops**: 4× mechanical switches (NO, normally-open)
- **E-Stop**: 1× mushroom button (NC, normally-closed) *optional*
- **Cooling**: Heatsinks (4×) + 40mm fan for NEMA drivers
- **Capacitors**: 100µF electrolytic (VMOT rail)
- **Wiring**: 22AWG for motors, jumper wires for signals
- **Connectors**: JST-XH or screw terminals for motors

---

## Repository Structure

```
roboarm-project/
├── arduino_uno/
│   ├── uno_roboarm.ino        # Main sketch
│   ├── config_pins.h          # Pin definitions
│   └── config_motion.h        # Motor parameters
├── esp01s/
│   ├── platformio.ini         # PlatformIO config
│   └── src/
│       ├── main.cpp           # ESP firmware
│       └── config.h           # WiFi & UART settings
├── TESTING_GUIDE.md           # Step-by-step validation
├── WIRING_DIAGRAM.md          # Connection reference
├── TROUBLESHOOTING.md         # Common issues
└── README.md                  # This file
```

---

## Quick Start

### 1. Hardware Assembly
1. **Mount CNC Shield** on Arduino Uno
2. **Insert DRV8825 drivers** (mind orientation! Enable pin toward board edge)
3. **Set microstepping jumpers**:
   - X/Y (NEMA): M0, M1, M2 all ON → 1/16 step
   - Z/A (28BYJ): No jumpers → Full step (or M0 only → 1/4 step)
4. **Wire motors** per [WIRING_DIAGRAM.md](docs/WIRING_DIAGRAM.md)
5. **Connect ESP-01S** to dedicated 3.3V supply
6. **Wire UART**: ESP TX→Uno RX, ESP RX→Uno TX, common GND

### 2. Calibrate DRV8825 Current
**Critical step!** Use multimeter on Vref test point:
- **NEMA-17**: 0.60-0.70V (1.2-1.4A with R100 sense resistors)
- **28BYJ-48**: 0.05-0.075V (0.10-0.15A)

See [TESTING_GUIDE.md Phase 1.2](docs/TESTING_GUIDE.md#12-set-vref-critical) for details.

### 3. Upload Firmware

**Arduino Uno:**
```bash
# Using Arduino IDE
1. Open arduino_uno/uno_roboarm.ino
2. Select Board: "Arduino Uno"
3. Verify SHIELD_VARIANT in config_pins.h matches your board
4. Upload
```

**ESP-01S:**
```bash
# Using PlatformIO
cd esp01s
# Edit src/config.h: Set WIFI_SSID and WIFI_PASSWORD
pio run -t upload

# Or Arduino IDE:
1. Open esp01s/src/main.cpp
2. Board: "Generic ESP8266 Module"
3. Flash Size: "1MB (FS:64KB OTA)"
4. Upload Speed: 115200
5. **Disconnect UART from Uno during upload!**
6. Upload
```

### 4. Configure & Test
1. **Check Shield Variant**: Measure A-axis pins with multimeter
   - D12/D13: Set `SHIELD_VARIANT 1`
   - A3/A4: Set `SHIELD_VARIANT 2`
2. **Power on & verify**:
   - Uno Serial Monitor: Should see init messages
   - ESP LED: Solid ON when WiFi connected
3. **Find ESP IP**: Check router DHCP table or serial monitor
4. **Open Web UI**: Browse to `http://<ESP_IP>/`
5. **Run tests**: Follow [TESTING_GUIDE.md](docs/TESTING_GUIDE.md)

---

## Configuration

### Motor Parameters (`config_motion.h`)
```cpp
// Adjust these after mechanical assembly:
const long J_MIN_LIMIT[4] = { -20000, -15000, 0, 0 };
const long J_MAX_LIMIT[4] = {  20000,  15000, 4000, 4000 };

// Tune for smoothness:
const float J_MAX_SPEED[4] = { 2000, 2000, 400, 400 };
const float J_MAX_ACCEL[4] = { 1500, 1500, 300, 300 };

// If motor runs backward:
const bool J_DIR_INVERT[4] = { false, false, false, false };
```

### WiFi & Network (`esp01s/src/config.h`)
```cpp
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
#define DEVICE_NAME "RoboArm"

#define PACKET_RATE_HZ 30  // 20-50 recommended
```

### Pin Mapping
Automatically configured based on `SHIELD_VARIANT` in `config_pins.h`. See pin reference table below.

---

## Pin Reference

### CNC Shield v3 (Variant 1 - Common)
| Signal | Arduino Pin | Shield Label | Notes |
|--------|-------------|--------------|-------|
| X_STEP | D2 | X-STEP | J1 (Base) |
| X_DIR | D5 | X-DIR | |
| Y_STEP | D3 | Y-STEP | J2 (Shoulder) |
| Y_DIR | D6 | Y-DIR | |
| Z_STEP | D4 | Z-STEP | J3 (Wrist) |
| Z_DIR | D7 | Z-DIR | |
| A_STEP | D12 | A-STEP | J4 (Gripper) |
| A_DIR | D13 | A-DIR | LED mirrors DIR |
| EN (all) | D8 | EN | Active-LOW enable |

### Endstops (INPUT_PULLUP, NO→GND)
| Joint | Pin | Connection |
|-------|-----|------------|
| X_MIN | D9 | J1 base minimum |
| Y_MIN | D10 | J2 shoulder minimum |
| Z_MIN | D11 | J3 wrist minimum |
| A_MIN | A0 | J4 gripper minimum |

### ESP-01S Connections
| ESP Pin | Connection | Notes |
|---------|------------|-------|
| VCC | 3.3V LDO | 500mA minimum |
| GND | Common GND | |
| TX (GPIO1) | Uno RX (D0) | UART to Uno |
| RX (GPIO3) | Uno TX (D1) | UART from Uno |
| CH_PD | 3.3V | Chip enable |
| GPIO2 | (Status LED) | Optional |
| GPIO0 | (floating) | Pull LOW for flash mode |

---

## Usage

### Web Interface

1. **Connect to WiFi network**
2. **Navigate to ESP IP address** in browser
3. **Interface elements**:
   - **Left Joystick**: Controls J1 (X-axis) and J2 (Y-axis)
   - **Right Joystick**: Controls J3 (X-axis) and J4 (Y-axis)
   - **DEADMAN Switch**: Must be ON to enable motion
   - **E-STOP Button**: Immediate halt, requires clear
   - **Enable/Disable**: Toggle driver power
   - **HOME All**: Run homing sequence
   - **Position Display**: Current step counts

### Operating Procedure

**First Power-On:**
1. Click **"HOME All"** button
2. Wait for all joints to find endstops (~30-60s)
3. "Homed" indicators turn green
4. Click **"Enable"** to power drivers
5. **Hold DEADMAN** switch
6. Move joysticks to control motion
7. Release joysticks → motion stops
8. Release DEADMAN → all motion inhibited

**Normal Operation:**
- Always hold DEADMAN during motion
- Watch position limits in UI
- Use E-STOP if unexpected behavior
- Clear E-STOP before resuming

---

## Communication Protocol

### Command Packets (ESP → Uno)
**Velocity Control (30 Hz):**
```json
{
  "t": 123456,
  "mode": "vel",
  "q": [-0.5, 0.7, 0.0, 0.3],
  "en": 1,
  "seq": 42
}
```
- `q`: Normalized velocities [-1.0, 1.0] for J1-J4
- `en`: Enable flag (1=deadman held, 0=released)
- `seq`: Sequence number (rolls over at 65535)

**Special Commands:**
```json
{"cmd": "home"}              // Start homing all joints
{"cmd": "estop"}             // Trigger software E-Stop
{"cmd": "clear_estop"}       // Clear E-Stop state
{"cmd": "enable", "val": 1}  // Enable drivers (0=disable)
```

### Status Telemetry (Uno → ESP, 10 Hz)
```json
{
  "t": 789012,
  "pos": [1250, -3400, 500, 800],
  "homed": [1, 1, 1, 1],
  "lim": [0, 0, 0, 0],
  "en": 1,
  "fault": 0,
  "mode": "vel",
  "seq_ack": 42
}
```

---

## Safety Features

### Timeout Protection
- **UART Timeout**: 300ms without valid packet → stop all motion
- **Watchdog**: ESP sends `en=0` if no UI activity for 150ms
- **Deadman Release**: Immediate stop when switch released

### Limit Protection
- **Soft Limits**: Configurable min/max positions per joint
- **Hard Endstops**: Mechanical switches clamp motion
- **Provisional Limits**: Conservative bounds when unhomed

### Emergency Stop
- **Software E-Stop**: UI button, immediate driver disable
- **Hardware E-Stop**: Optional NC loop on pin A1
- **Recovery**: Requires explicit "Clear" command + re-enable

### Fault Detection
- Hardware E-Stop trigger
- Homing timeout
- UART communication loss
- Endstop collision (if `ENDSTOP_AS_ESTOP` enabled)

---

## Troubleshooting

### Common Issues

**Motor doesn't move:**
- Check Vref (must be >0V)
- Verify driver orientation (enable pin correct)
- Test with multimeter: STEP pin should pulse
- Confirm motor wiring (continuity test)

**Motor runs backward:**
- Set `J_DIR_INVERT[i] = true` in `config_motion.h`
- Or swap A1↔A2 or B1↔B2 coil wires

**Stuttering/missed steps:**
- Increase Vref by 0.05V
- Reduce `J_MAX_SPEED[i]`
- Check mechanical binding
- Verify microstepping jumpers

**Web UI won't load:**
- Verify ESP connected to WiFi (LED solid ON)
- Check router for ESP IP address
- Try direct IP instead of hostname
- Ping ESP from computer

**Homing fails:**
- Check endstop wiring (NO→GND)
- Verify `HOMING_DIR[i]` matches physical layout
- Test endstop manually (short to GND, check telemetry)
- Increase `HOMING_TIMEOUT_MS`

**UART timeout errors:**
- Verify TX↔RX crossover (not TX→TX)
- Check baud rate matches (115200 both sides)
- Ensure common ground
- Close Serial Monitor if open

See [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for detailed solutions.

---

## Performance Specifications

### Motion
- **Velocity Range**: 0-2000 steps/s (NEMA), 0-400 steps/s (28BYJ)
- **Acceleration**: 300-1500 steps/s² (configurable)
- **Position Resolution**: 
  - NEMA @ 1/16: 0.05625° per step
  - 28BYJ @ full: 0.176° per step (after gearbox)
- **Response Latency**: <100ms (joystick to motion)

### Communication
- **UART Baud**: 115200 bps
- **Command Rate**: 30 Hz (configurable 20-50 Hz)
- **Telemetry Rate**: 10 Hz
- **Timeout**: 300ms

### Power
- **12V Rail**: 3-5A typical, 6A peak
- **5V (Uno)**: ~100mA
- **3.3V (ESP)**: ~80mA active, 150mA peak (WiFi TX)

---

## Known Limitations

1. **28BYJ Backlash**: ~5-15° due to plastic gears. Acceptable for low-precision axes.
2. **ESP-01S RAM**: Limited heap; UI must stay minimal. Upgrade to ESP-12 if adding features.
3. **No Kinematics**: Joint control only (no Cartesian/trajectory planning in current version).
4. **Single UART**: Cannot use Serial Monitor on Uno during operation with ESP connected.
5. **Homing Required**: Soft limits disabled until homed (or use provisional limits).

---

## Future Enhancements

### Planned Features
- [ ] Inverse kinematics (joint → Cartesian)
- [ ] Preset position macros
- [ ] Multi-point trajectory recording/playback
- [ ] Mobile app (native Android/iOS)
- [ ] SD card logging
- [ ] PID tuning for closed-loop control (with encoders)

### Hardware Upgrades
- [ ] Upgrade to ESP-12/NodeMCU (more GPIO, RAM)
- [ ] Add rotary encoders for position feedback
- [ ] Implement current sensing (motor load monitoring)
- [ ] TMC2209 drivers (quieter, sensorless homing)

---

## Contributing

Contributions welcome! Please:
1. Test changes on hardware before PR
2. Document new parameters in config files
3. Update TESTING_GUIDE.md if adding features
4. Follow existing code style

---

## License

MIT License - see LICENSE file for details.

**Hardware designs and mechanical assemblies are user-provided.**

---

## Support & Resources

### Documentation
- [Testing Guide](docs/TESTING_GUIDE.md) - Complete validation procedure
- [Wiring Diagram](docs/WIRING_DIAGRAM.md) - Connection reference
- [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues

### Libraries Used
- [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/) - Stepper control
- [ArduinoJson](https://arduinojson.org/) - JSON parsing
- [GyverHub](https://github.com/GyverLibs/GyverHub) - Web UI framework

### External Resources
- [DRV8825 Datasheet](https://www.ti.com/lit/ds/symlink/drv8825.pdf)
- [28BYJ-48 Bipolar Conversion](https://coeleveld.com/wp-content/uploads/2016/10/Modifying-a-28BYJ-48-step-motor-for-bipolar-drive.pdf)
- [CNC Shield v3 Wiki](https://github.com/gnea/grbl/wiki/Connecting-Grbl)

---

## Acknowledgments

Project structure based on industrial motion control best practices. Special thanks to the AccelStepper, GyverHub, and Arduino communities.

**Built with ❤️ for makers and robotics enthusiasts.**

---

## Safety Notice

⚠️ **This system controls motors with significant torque.** Always:
- Test in safe environment with clear workspace
- Use appropriate mechanical limits and guards
- Never bypass E-Stop systems
- Keep body parts away from moving components
- Have emergency power cutoff accessible

**The developers assume no liability for damage or injury resulting from use of this system.**