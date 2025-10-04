# Wiring Diagram & Connection Reference

## Overview Schematic

```
                    ┌─────────────────────────────────────┐
                    │         12V PSU (5A min)            │
                    └──────┬──────────────────────┬───────┘
                           │                      │
                           │ VMOT              GND│
                           │                      │
                    ┌──────▼──────────────────────▼───────┐
                    │      CNC Shield v3 (on Uno)         │
                    │  ┌───┬───┬───┬───┐                  │
                    │  │DRV│DRV│DRV│DRV│  [X][Y][Z][A]    │
                    │  │ X │ Y │ Z │ A │  Drivers         │
                    │  └─┬─┴─┬─┴─┬─┴─┬─┘                  │
                    │    │   │   │   │                     │
                    │ ┌──▼───▼───▼───▼──┐                 │
                    │ │  Arduino Uno    │                 │
                    │ │   ATmega328P    │                 │
                    │ └──┬───────────┬──┘                 │
                    │    │D0(RX) D1(TX)                    │
                    └────┼───────────┼────────────────────┘
                         │           │
                         │  UART     │
                         ▼           ▼
                    ┌────────────────────────┐
                    │      ESP-01S           │
                    │   (ESP8266 WiFi)       │
                    │  GPIO3(RX) GPIO1(TX)   │
                    └────┬──────────┬────────┘
                         │          │
                    3.3V │      GND │
                    ┌────▼──────────▼────┐
                    │   AMS1117-3.3 LDO  │
                    │   (from 5V Uno)    │
                    └────────────────────┘

Motors:
  X-axis → NEMA-17 #1 (Base J1)
  Y-axis → NEMA-17 #2 (Shoulder J2)
  Z-axis → 28BYJ-48 #1 (Wrist J3) - BIPOLAR
  A-axis → 28BYJ-48 #2 (Gripper J4) - BIPOLAR
```

---

## Detailed Connections

### 1. Power Distribution

#### 12V Main Rail
```
PSU 12V+ ───┬──→ CNC Shield VMOT
            │
            └──→ 100µF Capacitor to GND (close to shield)

PSU GND ────┴──→ Common GND (shield, Uno, ESP, endstops)
```

**Requirements:**
- PSU: 12V DC, 5A minimum (60W+)
- Use 18-20 AWG wire for power
- Keep VMOT wire runs short (<30cm)
- Add 100µF electrolytic capacitor at shield VMOT terminal

#### 5V for Uno
```
CNC Shield → Arduino Uno (via stacking headers)
Uno 5V pin → Use for LDO input (if external LDO)
```

#### 3.3V for ESP-01S
```
┌─────────────────────────────────┐
│  3.3V LDO Module (AMS1117-3.3)  │
│                                 │
│  VIN (5V) ←─── Uno 5V pin       │
│  GND      ←─── Common GND       │
│  VOUT     ───→ ESP VCC          │
│                                 │
│  + 10µF cap on output           │
│  + 0.1µF ceramic cap near ESP   │
└─────────────────────────────────┘
```

**Critical:**
- **DO NOT** use Uno's 3.3V pin (insufficient current, <50mA)
- LDO must handle 500mA minimum
- Add decoupling capacitors close to ESP-01S
- If using separate 3.3V supply, tie GND to common

---

### 2. CNC Shield v3 Driver Installation

#### DRV8825 Orientation
```
┌─────────────────┐
│  CNC Shield v3  │
│                 │
│  [X] [Y] [Z] [A]  ← Driver sockets
│   │   │   │   │   
│  ┌▼┐ ┌▼┐ ┌▼┐ ┌▼┐
│  │ │ │ │ │ │ │ │  ← Potentiometers face OUTWARD
│  │▪│ │▪│ │▪│ │▪│  ← Enable pin toward board EDGE
│  └─┘ └─┘ └─┘ └─┘
│   ▲               ← GND pins (marked on shield)
│                 │
└─────────────────┘

EN pin location (look for this marking on driver):
    ┌────────┐
    │ ENABLE │  ← This side toward board edge
    ├────────┤
    │        │
    │ DRV    │
    │ 8825   │
    └────────┘
```

**Verification:**
1. Insert driver WITHOUT bending pins
2. Enable pin should align with "EN" silk on shield
3. Potentiometer accessible for Vref adjustment
4. All 16 pins fully seated

#### Microstepping Jumper Configuration

**NEMA-17 Joints (X, Y) - 1/16 Microstepping:**
```
X-axis socket:          Y-axis socket:
 [●] M0                 [●] M0
 [●] M1                 [●] M1
 [●] M2                 [●] M2
 
● = Jumper installed
All three jumpers ON = 1/16 step
```

**28BYJ Joints (Z, A) - Full Stepping:**
```
Z-axis socket:          A-axis socket:
 [ ] M0                 [ ] M0
 [ ] M1                 [ ] M1
 [ ] M2                 [ ] M2
 
All jumpers OFF = Full step (recommended for 28BYJ)

Alternative (if smoother needed):
 [●] M0                 [●] M0
 [ ] M1                 [ ] M1
 [ ] M2                 [ ] M2
 
M0 only = 1/4 step
```

---

### 3. Motor Wiring

#### NEMA-17 (JK42HS40-1704) - 4-wire

**Standard color code:**
```
Motor Wire Colors → DRV8825 Terminal

  Black  ───────→ A1 (or 1A)
  Green  ───────→ A2 (or 2A)
  Red    ───────→ B1 (or 1B)
  Blue   ───────→ B2 (or 2B)
```

**Coil identification (use multimeter):**
- Coil A: Black ↔ Green (typically 2-4Ω)
- Coil B: Red ↔ Blue (typically 2-4Ω)
- No continuity between coils

**If direction is wrong:**
- Swap A1 ↔ A2 (Black ↔ Green) **OR**
- Set `J_DIR_INVERT[i] = true` in firmware

#### 28BYJ-48 Bipolar Conversion

**Original motor (before modification):**
```
5-wire unipolar:
  Red   ─┐
  Pink  ─┤
  Yellow─┤─→ [PCB]
  Orange─┤
  Blue  ─┘
```

**Conversion steps:**
1. **Remove plastic blue cap** (4 clips, gentle prying)
2. **Locate PCB** inside (small circular board)
3. **Cut trace** connecting Red wire to center tap (use knife/dremel)
4. **Verify isolation** with multimeter: Red should have NO continuity to any other wire
5. **Reassemble** motor

**After conversion (4-wire bipolar):**
```
Motor Wire → DRV8825 Terminal

  Blue   ───────→ A1
  Pink   ───────→ A2
  Yellow ───────→ B1
  Orange ───────→ B2
  Red    ───────→ (unused, insulate with tape)
```

**Coil verification:**
- Coil A: Blue ↔ Pink (~20Ω with gearbox)
- Coil B: Yellow ↔ Orange (~20Ω)
- Red: No continuity to anything

**Direction correction:**
- If backward: Swap Blue ↔ Pink (A1 ↔ A2)

---

### 4. Endstop Wiring

#### Connection Scheme (Normally-Open to GND)
```
Each endstop:

  [Switch]
     │
     ├─── NO (Normally Open)
     │
    ─┴─── COM (Common) ──→ GND
    
Arduino Pin ──→ NO terminal

When NOT pressed: Arduino pin = HIGH (internal pullup)
When pressed:     Arduino pin = LOW (shorted to GND)
```

#### Pin Assignment (Variant 1)
```
X_MIN (D9)  ────┬─[Switch]─┬──→ GND
                 NC        NO

Y_MIN (D10) ────┬─[Switch]─┬──→ GND

Z_MIN (D11) ────┬─[Switch]─┬──→ GND

A_MIN (A0)  ────┬─[Switch]─┬──→ GND
```

**Switch recommendations:**
- Mechanical microswitch (lever or roller)
- Rated: 5V, 0.1A minimum
- Mounting: Adjustable position for homing precision

#### Optional: Hardware E-Stop (NC Loop)
```
E-Stop mushroom button (Normally-Closed):

  A1 (Arduino) ───[NC Switch]─── GND
                     Closed = System OK
                     Open   = E-Stop triggered

Use NC (normally-closed) contacts:
  - Circuit closed = pin reads LOW (OK)
  - Button pressed = circuit opens = pin reads HIGH (E-Stop)
```

---

### 5. UART Cross-Connection (ESP ↔ Uno)

```
┌──────────────┐              ┌──────────────┐
│   ESP-01S    │              │  Arduino Uno │
│              │              │              │
│  GPIO1 (TX) ─┼──────────────┼─→ D0 (RX)    │
│              │    Data →    │              │
│  GPIO3 (RX) ─┼──────────────┼─← D1 (TX)    │
│              │    ← Data    │              │
│  GND        ─┼──────────────┼─ GND         │
│              │   Common     │              │
└──────────────┘              └──────────────┘

CRITICAL: TX → RX, RX → TX (crossover)
         NOT TX → TX !
```

**Wire recommendations:**
- Use 22-24 AWG for signal integrity
- Keep runs <30cm if possible
- Twist TX/RX pair together to reduce EMI
- **Disconnect during firmware upload** to either device

---

### 6. Complete Pin Map Table

#### Arduino Uno Connections

| Pin | Function | Connection | Notes |
|-----|----------|------------|-------|
| D0 | RX | ESP TX (GPIO1) | UART receive |
| D1 | TX | ESP RX (GPIO3) | UART transmit |
| D2 | X_STEP | CNC Shield | J1 step pulses |
| D3 | Y_STEP | CNC Shield | J2 step pulses |
| D4 | Z_STEP | CNC Shield | J3 step pulses |
| D5 | X_DIR | CNC Shield | J1 direction |
| D6 | Y_DIR | CNC Shield | J2 direction |
| D7 | Z_DIR | CNC Shield | J3 direction |
| D8 | EN (all) | CNC Shield | Driver enable (active-LOW) |
| D9 | X_MIN | Endstop switch | J1 min limit |
| D10 | Y_MIN | Endstop switch | J2 min limit |
| D11 | Z_MIN | Endstop switch | J3 min limit |
| D12 | A_STEP | CNC Shield | J4 step (Variant 1) |
| D13 | A_DIR | CNC Shield | J4 direction + LED |
| A0 | A_MIN | Endstop switch | J4 min limit (Variant 1) |
| A1 | ESTOP_IN | HW E-Stop | Optional NC loop |
| 5V | Power | LDO input | For ESP regulator |
| GND | Ground | Common GND | All subsystems |

#### ESP-01S Connections

| Pin | Function | Connection | Notes |
|-----|----------|------------|-------|
| VCC | Power | 3.3V LDO | **Not** Uno 3.3V pin! |
| GND | Ground | Common GND | |
| GPIO0 | Flash/Boot | (floating) | LOW = flash mode |
| GPIO1 (TX) | UART TX | Uno RX (D0) | Data to Uno |
| GPIO2 | Status LED | (built-in) | Optional indicator |
| GPIO3 (RX) | UART RX | Uno TX (D1) | Data from Uno |
| CH_PD | Chip Enable | 3.3V | Tie HIGH |
| RST | Reset | 3.3V or (floating) | Optional pullup |

---

### 7. Wire Gauge & Length Recommendations

| Connection | Wire Gauge | Max Length | Notes |
|------------|------------|------------|-------|
| 12V Power (PSU→Shield) | 18-20 AWG | 1m | Minimize voltage drop |
| Motor wires (4-wire) | 22 AWG | 2m | Twisted pairs preferred |
| Endstop signals | 24-26 AWG | 3m | With ground return |
| UART (ESP↔Uno) | 22-24 AWG | 30cm | Keep short, twist pair |
| Ground returns | 20 AWG | As needed | Star topology from PSU |

---

### 8. Cooling & Thermal Management

#### Heatsink Placement
```
CNC Shield (top view):

  [X]   [Y]   [Z]   [A]
   ▓     ▓     ·     ·    ▓ = Heatsink required (NEMA)
                           · = Optional (28BYJ)
NEMA drivers get HOT!
- Adhesive heatsinks: 14×14×10mm minimum
- Thermal paste or thermal tape
- Ensure full contact with driver IC
```

#### Fan Installation
```
40mm Fan (12V) mounted above X/Y drivers:

        ┌─────┐
        │ FAN │  ← Airflow DOWN onto heatsinks
        └─────┘
           ║
           ▼
      [X]  [Y]  [Z]  [A]

Wire fan to 12V rail with optional 100Ω resistor
  (quieter, still adequate cooling at ~70% speed)
```

---

### 9. Assembly Checklist

Before powering on:
- [ ] All drivers inserted correctly (enable pin orientation)
- [ ] Microstepping jumpers set per motor type
- [ ] Motor coils verified with multimeter (continuity)
- [ ] 28BYJ motors converted and tested
- [ ] Endstops wired NO→GND with pullup enabled
- [ ] UART crossed over (TX→RX, not TX→TX)
- [ ] ESP on dedicated 3.3V LDO (not Uno 3.3V pin)
- [ ] Common ground connected across all systems
- [ ] 100µF capacitor on VMOT rail
- [ ] Heatsinks installed on X/Y drivers
- [ ] No short circuits (visual inspection + multimeter)
- [ ] All connectors secure (no loose wires)

---

### 10. Verification Tests (Powered Off)

**Continuity Tests:**
1. VMOT to GND: **OPEN** (no short)
2. Motor coil A: **2-20Ω** (depending on motor)
3. Motor coil B: **2-20Ω**
4. Coil A to Coil B: **OPEN** (isolated)
5. Endstop NO to COM: **OPEN** when not pressed
6. Endstop NO to COM: **SHORT** when pressed
7. ESP VCC to GND: **OPEN**
8. UART TX/RX: **Not shorted** to VCC or GND

**Visual Inspection:**
- No bent pins on drivers or headers
- Capacitor polarity correct (stripe = negative)
- No solder bridges on custom boards
- Wire insulation intact (no exposed copper)
- Strain relief on motor wires

---

### 11. First Power-On Procedure

1. **Disconnect all motors** from drivers
2. **Set Vref to 0V** (turn pots fully CCW)
3. **Apply 12V power** (no load)
4. **Check voltages:**
   - VMOT: 12V ±0.3V
   - Uno 5V: 4.8-5.2V
   - ESP 3.3V: 3.2-3.4V
5. **If OK, power off**
6. **Set Vref** per calibration procedure (see TESTING_GUIDE Phase 1.2)
7. **Connect motors** one at a time
8. **Test each motor** individually
9. **Full system test** only after individual verification

---

## Troubleshooting Wiring Issues

### No Power
- Check PSU output voltage (should be 12V)
- Verify fuse in PSU or inline fuse
- Measure VMOT at shield terminals
- Check for loose barrel jack or screw terminals

### Driver Not Responding
- Verify enable pin orientation (correct insertion)
- Check EN pin (D8) is LOW when enabled
- Measure Vref (should be >0V)
- Look for LED indicator on driver board

### Motor Hums But Doesn't Move
- **Vref too low**: Increase by 0.05V
- **Wrong microstepping**: Check jumpers
- **Coil miswired**: Verify A1/A2, B1/B2 pairing
- **Mechanical binding**: Disconnect and test free rotation

### Motor Runs Backward
- Firmware: Set `J_DIR_INVERT[i] = true`
- **OR** Hardware: Swap A1↔A2 wires
- Do **not** swap both coils (A↔B) - will cause incorrect motion

### ESP Won't Connect
- Verify 3.3V at VCC pin (not 5V!)
- Check CH_PD tied HIGH
- GPIO0 should be floating (not LOW)
- Try manual reset (short RST to GND briefly)

### UART No Communication
- Verify crossover: ESP TX → Uno RX
- Check baud rate matches (115200)
- Disconnect Serial Monitor if open on Uno
- Test with multimeter: Should see voltage changes on TX lines

### Endstop Always Triggered
- Check for short to GND
- Verify switch is NO (normally open), not NC
- Test switch with multimeter in isolation
- Ensure INPUT_PULLUP enabled in firmware

### Thermal Shutdown
- Driver too hot (>85°C): Reduce Vref, add cooling
- PSU overheating: Check current draw vs rating
- Motor too hot (>70°C): Reduce speed or Vref

---

## Safety Notes

### High Current Paths
- 12V VMOT can supply 5A+ (60W)
- Short circuit **will** damage components
- Use fuse: 5A slow-blow in PSU line
- Never hot-plug motors (power off first)

### Driver Protection
- **Never** remove/insert drivers with power on
- Set Vref **before** connecting motors
- Start conservative (low Vref), increase gradually
- Monitor temperature during first runs

### ESP Protection
- **3.3V only!** 5V will destroy ESP-01S
- Use LDO with overcurrent protection
- Add 0.1µF ceramic cap **close** to VCC pin
- Avoid static discharge (handle by edges)

### Motor Considerations
- Stepper motors generate back-EMF
- Drivers have protection, but add decoupling
- 28BYJ plastic gears: Don't exceed 0.08V Vref
- Mechanical endstops must be properly mounted

---

## Recommended Tools

### For Assembly
- Soldering iron (if making custom cables)
- Wire strippers (22-24 AWG)
- Crimpers for JST/Dupont connectors
- Small screwdrivers (Phillips, flat)
- Needle-nose pliers
- Multimeter (essential!)

### For Calibration
- Digital multimeter with 0.01V resolution
- Ceramic screwdriver (for Vref adjustment)
- Infrared thermometer (for thermal check)
- Oscilloscope (optional, for debugging signals)

### For Maintenance
- Isopropyl alcohol (cleaning flux)
- Thermal paste (re-applying heatsinks)
- Spare jumper wires
- Spare drivers (DRV8825 can fail)

---

## Appendix: 28BYJ-48 Bipolar Conversion Photos

```
BEFORE Conversion (Unipolar):
   _______________
  |   28BYJ-48    |
  |   [Gear Box]  |
  |_______________|
       |||||        5 wires out
       |||||
   Red (center tap) ← Must cut internal trace

PCB Inside (after removing cap):
       ┌─────────┐
       │    ●    │ ← Cut this copper trace
   ────┤ [Coils] ├──── (connects red wire to center)
       │         │
       └─────────┘

AFTER Conversion (Bipolar):
   Only 4 wires active:
   Blue, Pink, Yellow, Orange
   Red: Insulated, not connected
```

---

## Reference Links

### Datasheets
- [DRV8825 Datasheet (TI)](https://www.ti.com/lit/ds/symlink/drv8825.pdf)
- [28BYJ-48 Specifications](http://www.4tronix.co.uk/arduino/Stepper-Motors.php)
- [AMS1117-3.3 Datasheet](https://www.advanced-monolithic.com/pdf/ds1117.pdf)

### Guides
- [28BYJ Bipolar Conversion](https://coeleveld.com/wp-content/uploads/2016/10/Modifying-a-28BYJ-48-step-motor-for-bipolar-drive.pdf)
- [DRV8825 Current Limiting](https://lastminuteengineers.com/drv8825-stepper-motor-driver-arduino-tutorial/)
- [CNC Shield Pinout](https://blog.protoneer.co.nz/arduino-cnc-shield/)

---

## Contact & Support

For wiring-specific questions:
1. Check continuity with multimeter **before** applying power
2. Verify all voltages match specifications
3. Consult TESTING_GUIDE.md for systematic validation
4. See TROUBLESHOOTING.md for common issues

**Never apply power if unsure about connections!**

---

**Document Version:** 1.0  
**Last Updated:** 2025-10-04  
**Verified On:** CNC Shield v3.0, Arduino Uno R3, ESP-01S (black 1MB)