# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Building and Upload
```bash
# Build the project
pio run

# Build and upload to ESP32
pio run --target upload

# Monitor serial output (115200 baud)
pio device monitor

# Build, upload, and monitor in one command
pio run --target upload && pio device monitor
```

### Development Workflow
```bash
# Clean build (if needed)
pio run --target clean

# Check for syntax/compilation issues
pio check

# Update library dependencies
pio lib update
```

## Architecture Overview

This is an ESP32-based IoT plant lighting controller with a component-based architecture. The system combines time-based scheduling with ambient light detection to intelligently control plant lights.

### Core Components

**Main Integration** (`src/main.cpp`)
- System orchestration and status display
- Component lifecycle management
- Main control loop with status monitoring

**PlantController** (`src/plantcontroller.cpp`, `include/plantcontroller.h`)
- Central decision engine that integrates all components
- Two-stage decision logic: schedule check → light level check
- Health monitoring and comprehensive logging
- Core business logic for when lights should be on/off
- Decisions are expressed via `ControlDecision` (`TurnOn`/`TurnOff`/`KeepCurrent`/`WaitForData`) paired with a `ControlReason` (e.g. `OutOfSchedule`, `InScheduleDark`, `SensorFailure`, `RelayBusy`) - use these enums rather than inventing new state when extending the decision logic

**Hardware Abstraction Layer:**
- **RelayController**: Safe relay switching with anti-flicker protection
- **LightSensor**: VEML7700 I2C sensor with averaging buffer; self-heals via `attemptRecovery()` after repeated consecutive read failures (I2C bus reset + sensor reinit)
- **WiFiManager**: Robust WiFi connection with auto-reconnection
- **TimeManager**: NTP synchronization with timezone support
- **WatchdogManager**: Hardware task watchdog (`esp_task_wdt`) that force-resets the board if `loop()` ever hangs (e.g. a stuck I2C read); only subscribed after the boot-time WiFi/time wait loops complete, to avoid false-triggering on a slow but healthy boot

**System-Level Components:**
- **SystemDiagnostics**: Boot count, unexpected-reboot detection (RTC crash marker), per-component failure/recovery history, WiFi/sensor performance metrics, persisted via the ESP32 Preferences API
- **ConfigManager**: Runtime configuration (WiFi credentials, schedule, threshold, hysteresis, timezone) persisted via Preferences, with fallback to `config.h` defaults
- **PlantWebServer**: Single-page status/config UI served over `ESPAsyncWebServer` when WiFi is connected - has no authentication and echoes the WiFi password into the settings form, so treat it as trusted-LAN-only

### Key Design Patterns

**Component Health Model**: Each component reports its health status, and PlantController only makes decisions when all required components are healthy.

**Safety-First Design**: System defaults to safe states during errors. RelayController has minimum switching intervals to prevent hardware damage.

**Configuration-Driven**: All settings centralized in `include/config.h` - WiFi credentials, schedules, thresholds, pins, etc.

**Rich Status Display**: Comprehensive emoji-based status output for easy debugging and monitoring.

## Configuration

**Critical Settings** (`include/config.h`):
- `WIFI_SSID` / `WIFI_PASSWORD`: Network credentials - only used as the fallback `ConfigManager` falls back to if no runtime config was ever saved via the web UI (Preferences takes precedence once set)
- `LIGHT_START_HOUR` / `LIGHT_END_HOUR`: Schedule default (supports overnight schedules); also overridable at runtime via `ConfigManager`
- `LIGHT_THRESHOLD_LUX`: Ambient light threshold default for turning lights on; also overridable at runtime
- Hardware pins: `RELAY_PIN`, `I2C_SDA_PIN`, `I2C_SCL_PIN`

**Key Constants**:
- `MIN_SWITCH_INTERVAL_MS`: Anti-flicker protection (60 seconds minimum)
- `CHECK_INTERVAL_MS`: Main control loop interval (30 seconds)
- `SENSOR_SAMPLES`: Averaging buffer size for stable readings
- `WATCHDOG_TIMEOUT_MS`: Hardware watchdog timeout - must stay above the worst-case blocking call in `loop()` (currently `WIFI_TIMEOUT_MS`'s 10s reconnect wait)

## Coding Conventions

- **CamelCase** for methods/variables, **PascalCase** for classes
- **Lowercase filenames** without underscores
- **Three slashes (`///`)** for documentation comments
- **Access via `this->`** pointer for class members
- **"We" voice** in comments explaining rationale
- **Modern C++** features: `[[nodiscard]]`, const correctness

## Hardware Requirements

- **ESP32 Development Board** (tested with LOLIN32)
- **VEML7700 Light Sensor** (I2C interface)
- **10A Relay Module** with optocoupler isolation
- **External 5V Power Supply** for relay (ESP32 USB 5V rail not available)

## Development Tips

**Serial Monitor**: System provides comprehensive status output with emoji indicators for quick visual debugging.

**Component Testing**: There is no unit test suite yet - `test/` and `lib/` only contain PlatformIO's default scaffolding READMEs. Validate behavior by checking existing test patterns in main.cpp (it doubles as a full integration test with status display functions) or `pio test` once a PlatformIO test suite is added.

**Error Handling**: System continues operating with degraded functionality when components fail. Check health status methods.

**Hardware Gotcha**: LOLIN32 board requires external 5V supply for relay module - 3.3V operation causes reliability issues.

## Integration Points

The PlantController is the integration hub. When adding new features:
1. Consider impact on the two-stage decision logic
2. Update health monitoring if adding new components  
3. Maintain the safety-first approach
4. Add appropriate status display output