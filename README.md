# Smart Plant Light Controller - Project Documentation

## Project Overview

The Smart Plant Light Controller is an ESP32-based system that intelligently manages plant lighting by combining time-based scheduling with ambient light detection. The system automatically turns plant lights ON/OFF based on both the time of day and current light conditions, optimizing energy efficiency while ensuring plants receive adequate lighting.

### Core Concept
- **Time-based scheduling**: Lights only operate within configured hours (e.g., 6:00-22:00)
- **Ambient light detection**: Within schedule hours, lights turn on only when ambient light is below threshold
- **Energy efficiency**: Prevents lights from running unnecessarily during bright daylight
- **Reliability**: Robust error handling and component health monitoring

## Hardware Components

### Core Electronics
- **ESP32 Development Board**: LOLIN32 ESP32-WROOM-32
- **Light Sensor**: VEML7700 ambient light sensor (I2C interface)
- **Relay Module**: 10A relay with PC814 optocoupler isolation
- **Power Supply**: 5V external adapter for relay, USB power for ESP32

### Connections
```
ESP32 Connections:
├── GPIO2 → Relay IN1 (control signal)
├── GPIO19 → VEML7700 SDA (I2C data)
├── GPIO22 → VEML7700 SCL (I2C clock)
├── 3.3V → VEML7700 VIN
└── GND → Common ground

External 5V Supply:
├── 5V+ → Relay VCC
└── 5V- → Relay GND + ESP32 GND
```

### Safety Features
- Modified power strip with relay switching only the hot/live wire
- Optocoupler isolation between ESP32 and relay
- Proper electrical enclosure (recommended)
- Fuse protection (recommended)

## Software Architecture

### Development Environment
- **Platform**: PlatformIO with ESP32 framework
- **Language**: C++ with Arduino framework
- **Libraries**: 
  - Adafruit VEML7700 Library
  - SNTP via the ESP32 Arduino core (built-in, no external NTP library)
  - WiFi (built-in)

### Coding Style Guidelines
- CamelCase for methods/variables, PascalCase for classes
- Lowercase filenames without underscores
- Access members via `this->` pointer
- Three slashes (`///`) for documentation comments
- Modern C++ features (`[[nodiscard]]`, const correctness)
- "We" voice in comments explaining rationale

### Component Architecture

The software follows a modular component-based architecture:

Each component's declaration lives in `include/<name>.h`, implementation in `src/<name>.cpp`:
```
main.cpp                # Main integration and system orchestration
relaycontroller.h/.cpp   # Safe relay switching with anti-flicker
lightsensor.h/.cpp       # VEML7700 interface with averaging
wifimanager.h/.cpp       # WiFi connection and reconnection logic
timemanager.h/.cpp       # NTP time synchronization and scheduling
plantcontroller.h/.cpp   # Core decision logic integrating all components
systemdiagnostics.h/.cpp # Boot count, crash detection, health/perf metrics
watchdogmanager.h/.cpp   # Hardware task watchdog (hang detection)
configmanager.h/.cpp     # Runtime configuration persistence (Preferences API)
webserver.h/.cpp         # Single-page status/config web UI (ESPAsyncWebServer)

include/config.h          # Default configuration constants (fallback values, no .cpp)
include/secrets.h         # WiFi credentials (gitignored, not in repo)
```

## Implemented Features

### ✅ Phase 1: Project Setup & Dependencies
- PlatformIO project configuration
- Library dependencies management
- Development environment setup
- Git repository structure

### ✅ Phase 2: Hardware Abstraction Layer
**RelayController Class:**
- Safe relay switching with minimum interval protection
- Anti-flicker logic preventing rapid state changes
- Emergency stop functionality
- Hardware state tracking and validation

**LightSensor Class:**
- VEML7700 sensor interface with I2C communication
- Circular buffer averaging for stable readings (configurable sample count)
- Threshold comparison for decision making
- Sensor health monitoring and error detection
- Support for 0.0036 lux/count resolution

### ✅ Phase 3: WiFi & Time Foundation
**WiFiManager Class:**
- Robust WiFi connection with automatic reconnection
- Exponential backoff for failed connection attempts
- Connection status monitoring and diagnostics
- Signal strength reporting and network health assessment

**TimeManager Class:**
- NTP time synchronization with configurable servers
- POSIX TZ string (`TIMEZONE_TZ`, automatic DST via libc)
- Schedule validation with day-boundary crossing support
- Time health monitoring and sync failure handling
- Automatic periodic resynchronization (background SNTP, non-blocking)

### ✅ Phase 4: Core Decision Logic
**PlantController Class:**
- Two-stage decision algorithm:
  1. **Schedule Check**: Is current time within configured window?
  2. **Light Check**: Is ambient light below threshold?
- Component health validation before decisions
- Comprehensive decision logging with reasons
- Manual override capability
- Performance metrics tracking (decisions made, relay changes)

### ✅ Phase 5: Advanced Integration
**SystemDiagnostics Class:**
- Comprehensive system health monitoring with uptime tracking
- Boot count and unexpected reboot detection (crash detection using RTC memory)
- Component failure history and recovery attempt tracking
- Performance metrics (WiFi signal quality average, sensor reading stability)
- Persistent diagnostics data stored in ESP32 flash memory (Preferences API)

**Automatic Error Recovery:**
- Light sensor: I2C bus reset and sensor reinitialization
- Time manager: Force NTP resync for time validation failures
- WiFi manager: Enhanced connection tracking (existing auto-reconnection)
- Recovery cooldown periods (60 seconds) to prevent rapid retry loops
- Comprehensive recovery logging and success rate tracking

**Enhanced Component Tracking:**
- WiFi: Connection success rate and attempt counting
- Time: Sync success rate and failed sync tracking
- Sensor: Consecutive failure counting for recovery triggering
- All components: Automatic recovery when failures detected by PlantController

**Watchdog Timer:**
- Hardware task watchdog (`esp_task_wdt`) force-resets the board if `loop()` ever hangs (e.g. a stuck I2C read) - the one failure mode SystemDiagnostics's after-the-fact crash detection can't recover from on its own
- Reports the hardware's actual reset reason (watchdog/panic/brownout/power-on/etc.) into SystemDiagnostics on boot
- Only subscribed after the boot-time WiFi/time wait loops complete, so a slow but healthy connection during startup doesn't false-trigger it

### ✅ Phase 6: Configuration Persistence & Web Interface
**ConfigManager Class:**
- Runtime configuration (WiFi credentials, schedule, threshold, hysteresis, min switch interval, timezone) persisted via the ESP32 Preferences API
- Settings survive reboots and power cycles; falls back to `config.h` defaults if nothing was ever saved
- Per-field range validation before a save is accepted

**PlantWebServer Class:**
- Single-page status/config UI served over `ESPAsyncWebServer` when WiFi is connected
- Real-time status display (schedule, light level, relay state, decision reason)
- Editable settings form (schedule, threshold, hysteresis, min switch interval, timezone, WiFi credentials)
- Manual override toggle (Auto / Force On / Force Off) via `ManualOverride`, bypassing schedule and light-level logic while still respecting the relay's minimum switch interval; not persisted - always resets to `Auto` on reboot
- **No authentication and echoes the current WiFi password into the settings form** - trusted-LAN-only, not intended to be exposed beyond the local network
- No REST/JSON API yet - the UI is server-rendered HTML forms only

**WiFi Credentials:**
- Moved out of `config.h` into a gitignored `secrets.h`; `config.h` keeps placeholder fallbacks for a fresh checkout

### ✅ Integration & System Features
- **Comprehensive Status Display**: Real-time system health monitoring
- **Automatic Error Recovery**: Self-healing component reinitialization
- **System Diagnostics**: Uptime, boot count, failure history, performance metrics
- **Safety-First Design**: Defaults to safe states during errors
- **Rich Debugging**: Detailed logging of all decisions and state changes
- **Visual Status Indicators**: Emoji-based status for quick recognition

## Configuration

### Key Settings (config.h)
These are only the fallback defaults used if no runtime configuration was ever
saved via the web UI - once saved, `ConfigManager`'s copy in flash (Preferences)
takes precedence. WiFi credentials live in the gitignored `secrets.h`, not here.
```cpp
// Hardware Configuration
#define RELAY_PIN 2
#define I2C_SDA_PIN 19  
#define I2C_SCL_PIN 22

// Network Configuration (fallback only - see secrets.h)
#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_TZ "CET-1CEST,M3.5.0,M10.5.0/3"  // Berlin, automatic DST

// Plant Light Schedule
#define LIGHT_START_HOUR 8   // 8:00 AM
#define LIGHT_END_HOUR 23    // 11:00 PM

// Light Sensor Configuration
#define LIGHT_THRESHOLD_LUX 100.0    // Turn on below this level
#define SENSOR_SAMPLES 5             // Averaging buffer size
#define CHECK_INTERVAL_MS 30000      // Check every 30 seconds

// Safety Configuration  
#define MIN_SWITCH_INTERVAL_MS 60000 // Min 1 minute between switches
#define WATCHDOG_TIMEOUT_MS 25000    // Must exceed WIFI_TIMEOUT_MS
```

## Hardware Lessons Learned

### Relay Module Power Requirements
- **Critical Issue**: LOLIN32 board does not expose USB 5V rail
- **Solution**: External 5V supply required for relay module
- **Problem**: 3.3V operation causes relay module failures over time
- **Recommendation**: Always use proper 5V supply for relay modules

### Component Reliability
- **Quality Matters**: Cheap relay modules may have reliability issues
- **Testing Approach**: Validate components individually before integration
- **Power Supply**: Adequate current capacity essential for reliable operation

## Roadmap

Known gaps and feature ideas live in [ROADMAP.md](ROADMAP.md), separate from
this file so it doesn't have to be re-read every time the backlog changes.

## Resuming Development

### Quick Start Checklist
1. **Hardware Setup**:
   ```bash
   # Verify connections:
   # - ESP32 powered via USB
   # - Relay powered via external 5V supply
   # - VEML7700 connected to I2C pins (19, 22)
   # - All grounds connected together
   ```

2. **Development Environment**:
   ```bash
   # Clone repository
   git clone <your-repo-url>
   cd smart-plant-light
   
   # Build and upload
   pio run --target upload
   pio device monitor
   ```

   **Wrong serial port picked?** macOS reassigns the `cu.usbserial-XXXX` suffix on
   every USB (re)plug/sleep-wake, so a stale port can linger in VS Code. Fix it
   in the **PlatformIO status bar at the bottom of VS Code** - click the port
   chooser there and select the current device (cross-check with `pio device
   list` if unsure which one is the board). `platformio.ini` also pins
   `upload_port`/`monitor_port` as a fallback, but the status bar picker takes
   priority when set.

3. **Configuration Updates**:
   - Set WiFi credentials in `include/secrets.h` (gitignored; create it if missing)
   - Schedule, threshold, hysteresis, etc. can be changed at runtime via the web UI without reflashing - `config.h` only supplies the defaults for a fresh device

### Testing Protocol
0. **Unit Tests**: `pio test -e native` runs the host-side tests for the
   pure decision logic in `src/controllogic.cpp` (schedule windows,
   hysteresis, override/schedule/sensor precedence)

1. **Component Tests**: Verify each component individually
   - Relay switching (GPIO control)
   - Light sensor readings (I2C communication)
   - WiFi connection (network connectivity)
   - Time synchronization (NTP)

2. **Integration Test**: Run full system and verify:
   - Automatic light control during schedule hours
   - Schedule override (lights off outside hours)
   - Component health monitoring
   - Error recovery behavior

3. **Long-term Reliability**: 
   - Monitor for hardware failures
   - Check power supply stability
   - Validate relay module reliability

### Development Priorities
System is stable and operational; diagnostics, watchdog, config persistence,
and the web UI (Phases 1-6) are done. See [ROADMAP.md](ROADMAP.md) for known
gaps and what to pick up next.

### Code Organization
- **Modular Design**: Each component in separate files
- **Clear Interfaces**: Well-defined component APIs
- **Comprehensive Logging**: Detailed debug output
- **Configuration Driven**: Easy parameter adjustment
- **Safety First**: Fail-safe defaults and error handling

### Performance Metrics
Current system achieves:
- **Response Time**: 30-second decision intervals
- **Accuracy**: ±0.1 lux light measurement precision
- **Reliability**: Automatic reconnection for network issues
- **Safety**: Multiple protection layers for electrical safety

## Project Success Metrics

The Smart Plant Light Controller successfully demonstrates:
- **Intelligent Automation**: Combines multiple sensor inputs for optimal decisions
- **Energy Efficiency**: Prevents unnecessary lighting during bright conditions  
- **Reliability**: Robust error handling and automatic recovery
- **Safety**: Multiple protection layers for electrical safety
- **Maintainability**: Clean, documented, modular code architecture
- **Extensibility**: Foundation for advanced features and home automation integration
