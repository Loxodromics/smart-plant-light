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
  - NTPClient Library
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

```
src/
├── main.cpp                 # Main integration and system orchestration
├── config.h                 # Configuration constants and settings
├── relaycontroller.h/.cpp   # Safe relay switching with anti-flicker
├── lightsensor.h/.cpp       # VEML7700 interface with averaging
├── wifimanager.h/.cpp       # WiFi connection and reconnection logic
├── timemanager.h/.cpp       # NTP time synchronization and scheduling
└── plantcontroller.h/.cpp   # Core decision logic integrating all components
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
- Timezone support (Berlin UTC+1)
- Schedule validation with day-boundary crossing support
- Time health monitoring and sync failure handling
- Automatic periodic resynchronization

### ✅ Phase 4: Core Decision Logic
**PlantController Class:**
- Two-stage decision algorithm:
  1. **Schedule Check**: Is current time within configured window?
  2. **Light Check**: Is ambient light below threshold?
- Component health validation before decisions
- Comprehensive decision logging with reasons
- Manual override capability
- Performance metrics tracking (decisions made, relay changes)

### ✅ Integration & System Features
- **Comprehensive Status Display**: Real-time system health monitoring
- **Error Recovery**: Graceful handling of component failures
- **Safety-First Design**: Defaults to safe states during errors
- **Rich Debugging**: Detailed logging of all decisions and state changes
- **Visual Status Indicators**: Emoji-based status for quick recognition

## Configuration

### Key Settings (config.h)
```cpp
// Hardware Configuration
#define RELAY_PIN 2
#define I2C_SDA_PIN 19  
#define I2C_SCL_PIN 22

// Network Configuration
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"
#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_OFFSET_HOURS 1  // Berlin = UTC+1

// Plant Light Schedule
#define LIGHT_START_HOUR 6   // 6:00 AM
#define LIGHT_END_HOUR 22    // 10:00 PM

// Light Sensor Configuration
#define LIGHT_THRESHOLD_LUX 100.0    // Turn on below this level
#define SENSOR_SAMPLES 5             // Averaging buffer size
#define CHECK_INTERVAL_MS 30000      // Check every 30 seconds

// Safety Configuration  
#define MIN_SWITCH_INTERVAL_MS 60000 // Min 1 minute between switches
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

## Upcoming Features

### Phase 5: Advanced Integration (Planned)
- **Advanced Error Recovery**: Automatic component reinitialization when failures detected
  - I2C bus reset and sensor reinitialization
  - WiFi reconnection with backoff strategies
  - Component health monitoring with automatic recovery attempts
- **System Diagnostics**: Enhanced logging and health reporting
  - Uptime tracking and crash detection
  - Component failure history
  - Performance metrics (WiFi signal, sensor stability)
- **Persistent Logging**: SD card or flash-based event logging for troubleshooting

### Phase 6: User Interface & Configuration (Future)
- **Configuration Persistence**: Save settings to ESP32 flash memory (Preferences API)
  - WiFi credentials storage
  - Light schedules and thresholds
  - Sensor calibration values
  - Settings survive reboots and power cycles
- **Web Interface**: Browser-based configuration and monitoring
  - Real-time system status dashboard
  - Editable configuration forms (schedules, thresholds, WiFi)
  - Light level and relay state visualization
  - Mobile-responsive design for smartphone access
- **REST API**: Programmatic access for external integration

### Phase 7: Home Automation Integration (Future)
- **MQTT Support**: Integration with Home Assistant, OpenHAB
- **Google Assistant/Alexa**: Voice control capabilities
- **Smartphone App**: Dedicated mobile application
- **Cloud Logging**: Historical data storage and analysis

### Phase 8: Advanced Features (Future)
- **Multiple Light Zones**: Control different plant areas independently
- **Sunrise/Sunset Simulation**: Gradual light transitions
- **Plant-Specific Profiles**: Customized lighting schedules per plant type
- **Weather Integration**: Adjust based on weather forecasts
- **Machine Learning**: Adaptive scheduling based on plant response

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

3. **Configuration Updates**:
   - Update WiFi credentials in `include/config.h`
   - Adjust schedule times for testing
   - Modify light threshold based on your environment

### Testing Protocol
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
1. **Current**: System is stable and operational
2. **Short-term**: Advanced error recovery and diagnostics (Phase 5)
3. **Medium-term**: Web interface and configuration persistence (Phase 6)
4. **Long-term**: Home automation integration (Phase 7)

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

This project serves as an excellent foundation for IoT automation systems, demonstrating professional software engineering practices, robust hardware integration, and intelligent decision-making algorithms.
