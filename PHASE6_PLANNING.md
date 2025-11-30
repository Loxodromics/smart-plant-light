# Phase 6 Implementation Planning

## Overview

Phase 6 focuses on **User Interface & Configuration**, transforming the system from a hardcoded device into a user-friendly, configurable IoT product.

**Current Limitation**: All configuration is hardcoded in `config.h`, requiring recompilation and reflashing for any setting change.

**Phase 6 Goal**: Enable runtime configuration, web-based monitoring, and flexible system management.

---

## Minimal Approach (Weekend Project)

### Scope: Core Configuration & Basic Web Interface

**Estimated Effort**: 1-2 days, ~850 lines of code

### Components

#### 1. ConfigManager Class
**File**: `include/configmanager.h`, `src/configmanager.cpp`

Store and retrieve configuration using ESP32 Preferences API (already proven in SystemDiagnostics).

**Configurable Settings**:
- WiFi credentials (SSID, password)
- Light schedule (start hour, end hour)
- Light threshold (lux value)
- Sensor sample count

**Features**:
- Load configuration on boot
- Save configuration to flash
- Fallback to config.h defaults if no saved settings
- Configuration validation

**Implementation Estimate**: ~200 lines

#### 2. Basic Web Interface
**File**: `include/webserver.h`, `src/webserver.cpp`

Single-page web dashboard using ESPAsyncWebServer.

**Dashboard Displays**:
- Current system status (relay state, light level, time, WiFi signal)
- Component health indicators
- System uptime and boot count
- Recent decision log

**Configuration Forms**:
- WiFi settings (SSID, password)
- Schedule settings (start hour, end hour)
- Light threshold (lux)
- Sensor configuration

**Features**:
- Static HTML with embedded CSS
- Form submission updates ConfigManager
- Immediate configuration application
- Mobile-friendly responsive layout

**Implementation Estimate**: ~400 lines (C++ server + HTML/CSS)

#### 3. Minimal REST API
**Endpoints**:
- `GET /api/status` - JSON with current system state
- `GET /api/config` - Retrieve current configuration
- `POST /api/config` - Update configuration (validates and saves)

**Implementation Estimate**: ~100 lines (integrated with web server)

#### 4. WiFi AP Fallback Mode
**Feature**: If WiFi connection fails, device becomes access point

**Configuration**:
- SSID: `PlantLight-Setup`
- Default IP: `192.168.4.1`
- No password (initial setup only)

**Use Case**: Initial device setup or WiFi network change

**Implementation Estimate**: ~150 lines

### Libraries Required

```ini
lib_deps =
    adafruit/Adafruit VEML7700 Library@^2.1.6
    arduino-libraries/NTPClient@^3.2.1
    me-no-dev/ESPAsyncWebServer@^1.2.3  # NEW
    me-no-dev/AsyncTCP@^1.1.1           # NEW
```

### Configuration Structure

```cpp
struct PlantLightConfig {
    // WiFi
    char wifiSSID[32];
    char wifiPassword[64];

    // Schedule (24-hour format)
    uint8_t lightStartHour;  // 0-23
    uint8_t lightEndHour;    // 0-23

    // Sensor
    float lightThresholdLux;
    uint8_t sensorSamples;   // 1-10

    // Validation flags
    bool isValid;
    uint32_t version;  // For future migration
};
```

### Integration Points

**Modified Classes**:
- `WiFiManager`: Accept runtime SSID/password from ConfigManager
- `TimeManager`: No changes needed
- `PlantController`: Read schedule and threshold from ConfigManager
- `LightSensor`: Accept runtime sample count from ConfigManager
- `main.cpp`: Initialize ConfigManager, start web server

### User Experience Flow

1. **First Boot (No Saved Config)**:
   - Device starts in AP mode (`PlantLight-Setup`)
   - User connects to WiFi network
   - Browser automatically opens configuration page
   - User enters WiFi credentials and basic settings
   - Device saves config, restarts, connects to WiFi
   - Web interface accessible via device IP

2. **Normal Operation**:
   - Device connects to WiFi using saved credentials
   - Web interface accessible at device IP
   - User can modify settings via forms
   - Changes take effect immediately (no reboot for most settings)

3. **WiFi Network Change**:
   - Device fails to connect
   - After timeout, reverts to AP mode
   - User connects and updates WiFi settings

---

## Feature-Complete Approach (Professional IoT Device)

### Scope: Rich Interface, Advanced Features, Security

**Estimated Effort**: 1-2 weeks, ~4300 lines of code

### Additional Components (Beyond Minimal)

#### 5. Advanced ConfigManager Features
- **Multiple Configuration Profiles**: Save/load plant-specific presets
- **Configuration Import/Export**: JSON backup/restore
- **Configuration History**: Track last 10 changes with timestamps
- **Factory Reset**: Restore defaults via web button
- **Implementation Estimate**: +300 lines

#### 6. Rich Web Dashboard
**File**: `include/websocketmanager.h`, `src/websocketmanager.cpp`

**Real-time Features**:
- WebSocket connection for live updates (no page refresh)
- Live light level gauge
- Relay state indicator with transition animations
- Connection quality meter

**Data Visualization**:
- Light level chart (last 24 hours)
- Relay state timeline
- WiFi signal strength graph
- Decision log with color-coded reasons

**Multi-page Interface**:
- Dashboard (status overview)
- Configuration (settings forms)
- Diagnostics (detailed health metrics)
- History (charts and logs)
- About (device info, firmware version)

**UI/UX**:
- Dark/Light theme toggle
- Mobile-responsive design (Bootstrap or similar)
- Progressive Web App (PWA) capabilities
- Offline status detection

**Implementation Estimate**: ~1500 lines (C++ + HTML + JavaScript)

#### 7. Comprehensive REST API
**Status Endpoints**:
- `GET /api/status` - Full system status
- `GET /api/components` - Individual component health
- `GET /api/diagnostics` - SystemDiagnostics data

**Configuration Endpoints**:
- `GET /api/config` - Current configuration
- `PUT /api/config` - Update configuration (validated)
- `GET /api/config/profiles` - List saved profiles
- `POST /api/config/profiles/{name}` - Save new profile
- `DELETE /api/config/profiles/{name}` - Delete profile

**Control Endpoints**:
- `POST /api/relay/override` - Manual relay control with timeout
- `POST /api/system/restart` - Reboot device
- `POST /api/system/factory-reset` - Reset to defaults

**Data Endpoints**:
- `GET /api/history/light?from={timestamp}&to={timestamp}` - Historical light readings
- `GET /api/history/relay` - Relay state changes
- `GET /api/logs?limit={n}` - Recent decision logs

**Implementation Estimate**: ~600 lines

#### 8. Authentication & Security
**File**: `include/authmanager.h`, `src/authmanager.cpp`

**Features**:
- Basic Authentication for web interface
- API token authentication for REST calls
- HTTPS support (self-signed certificate)
- Session management with timeout
- Optional IP whitelist
- CORS configuration for external access

**Implementation Estimate**: ~400 lines

#### 9. Advanced Features

**OTA Firmware Updates**:
- Upload new firmware via web interface
- Automatic backup of current firmware
- Rollback capability on failed update

**mDNS Support**:
- Access device via `plantlight.local` instead of IP address
- Automatic discovery on network

**Captive Portal**:
- When in AP mode, redirect all traffic to setup page
- Better user experience for initial setup

**Email Notifications**:
- Alert on critical errors (relay failure, sensor offline)
- Daily status summary
- Weekly diagnostics report

**Enhanced Scheduling**:
- Multiple schedule windows per day
- Different schedules for weekdays/weekends
- Vacation mode (disable automation)
- Sunrise/sunset based scheduling (future)

**Sensor Calibration**:
- Web-based calibration wizard
- Reference light source comparison
- Calibration history tracking

**Data Logging**:
- Store hourly readings to SPIFFS/LittleFS
- Circular buffer (7 days retention)
- Export logs as CSV

**Implementation Estimate**: ~1000 lines

#### 10. Integration Hooks
**File**: `include/integrationmanager.h`, `src/integrationmanager.cpp`

**Webhook Support**:
- POST to external URL on relay state changes
- Configurable retry logic
- Payload customization

**IFTTT Integration**:
- Trigger webhooks for external automation
- Applet templates for common scenarios

**Prometheus Metrics**:
- Export metrics in Prometheus format
- `/metrics` endpoint

**Syslog Support**:
- Send logs to remote syslog server
- Configurable log levels

**Implementation Estimate**: ~300 lines

### Additional Libraries Required

```ini
lib_deps =
    # Existing
    adafruit/Adafruit VEML7700 Library@^2.1.6
    arduino-libraries/NTPClient@^3.2.1

    # Phase 6 Core
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1

    # Phase 6 Advanced
    bblanchon/ArduinoJson@^6.21.0
    khoih-prog/ESP_DoubleResetDetector@^1.3.2
    # LittleFS (built-in to ESP32 framework)
```

### Memory Budget Analysis

**ESP32 Constraints**:
- Flash: 4MB (partitioned: 1MB app, 3MB storage)
- RAM: ~320KB total (~200KB available for user code)

**Memory Usage Estimates**:
- Minimal Approach: ~100KB flash, ~30KB RAM
- Feature-Complete: ~300KB flash, ~80KB RAM
- Historical Data Storage: ~500KB flash (SPIFFS)

**Optimization Strategies**:
- Store HTML/CSS/JS in SPIFFS (not program memory)
- Use PROGMEM for constant strings
- Limit concurrent WebSocket connections (max 4)
- Use circular buffers for logs
- Compress historical data
- Lazy-load web interface components

---

## Recommended Phased Approach

Implement Phase 6 incrementally to manage complexity:

### Phase 6A: Configuration Foundation
**Duration**: 1 weekend
**Components**: ConfigManager, WiFi AP fallback, basic web interface, minimal REST API
**Deliverable**: User can configure device without recompiling

### Phase 6B: Enhanced Interface
**Duration**: 1 week
**Components**: WebSocket updates, data visualization, mobile-responsive design, mDNS, OTA updates
**Deliverable**: Rich, real-time monitoring dashboard

### Phase 6C: Advanced Features
**Duration**: 1 week
**Components**: Authentication, data logging, multiple schedules, notifications, integration hooks
**Deliverable**: Professional-grade IoT device

---

## Architecture Recommendations

### New Component Structure

```
src/
├── main.cpp                      # (modified) Initialize new components
├── configmanager.cpp             # NEW: Configuration persistence
├── webserver.cpp                 # NEW: Web interface and REST API
├── websocketmanager.cpp          # NEW: Real-time updates (Phase 6B)
├── datalogger.cpp                # NEW: Historical data (Phase 6C)
├── authmanager.cpp               # NEW: Security (Phase 6C)
├── integrationmanager.cpp        # NEW: Webhooks/IFTTT (Phase 6C)
└── (existing components...)

include/
├── config.h                      # (modified) Add new component defaults
├── configmanager.h               # NEW
├── webserver.h                   # NEW
├── websocketmanager.h            # NEW
├── datalogger.h                  # NEW
├── authmanager.h                 # NEW
├── integrationmanager.h          # NEW
└── (existing headers...)

data/                             # NEW: Web interface files (SPIFFS)
├── index.html                    # Main dashboard
├── config.html                   # Configuration page
├── diagnostics.html              # Diagnostics page
├── history.html                  # Historical data page
├── styles.css                    # Shared styles
├── app.js                        # Frontend JavaScript
└── charts.min.js                 # Chart library (Chart.js)
```

### Component Interaction Flow

```
main.cpp
  ├── Initialize ConfigManager (loads saved config or defaults)
  ├── Pass config to existing components:
  │   ├── WiFiManager (SSID, password)
  │   ├── PlantController (schedule, threshold)
  │   └── LightSensor (sample count)
  ├── Initialize WebServer (serves interface, handles API)
  ├── Initialize WebSocketManager (real-time updates)
  └── Main loop:
      ├── PlantController.update()
      ├── WebSocketManager.broadcast(currentStatus)
      └── DataLogger.recordReading() (if enabled)
```

### Configuration Persistence Strategy

**Storage**: ESP32 Preferences API (key-value pairs in NVS flash)

**Namespace**: `plantlight`

**Keys**:
- `wifi.ssid` → WiFi SSID
- `wifi.pass` → WiFi password
- `sched.start` → Light start hour
- `sched.end` → Light end hour
- `sensor.thresh` → Light threshold (lux)
- `sensor.samples` → Sample count
- `config.version` → Config schema version

**Migration Strategy**: Version field allows future config updates with automatic migration.

### Web Interface Technology Stack

**Backend**: ESPAsyncWebServer
- Non-blocking async architecture
- Low memory footprint
- WebSocket support
- Static file serving from SPIFFS

**Frontend**: Lightweight vanilla JavaScript
- No heavy frameworks (React/Vue too large for ESP32)
- Chart.js for visualization (or lightweight alternative)
- Fetch API for REST calls
- WebSocket API for real-time updates

**Styling**: Custom CSS or minimal framework
- Avoid large CSS frameworks (Bootstrap ~150KB)
- Custom responsive grid (~5KB)
- CSS variables for theming

### Security Considerations

**Minimal Approach**: No authentication (assumes trusted local network)

**Feature-Complete Approach**:
- Basic Authentication for web interface
- API token for programmatic access
- Optional HTTPS (self-signed certificate)
- CORS headers configured appropriately
- Input validation on all API endpoints
- Rate limiting on authentication endpoints

**Future Enhancements**:
- OAuth2 integration
- Certificate pinning for HTTPS
- User role management

---

## Testing Strategy

### Phase 6A Testing
1. Configuration save/load after reboot
2. WiFi AP mode fallback
3. Web interface accessibility
4. Form validation and error handling
5. Configuration limits (invalid hours, negative thresholds)

### Phase 6B Testing
1. WebSocket connection stability
2. Real-time data updates
3. Chart rendering performance
4. Mobile device compatibility
5. OTA update process

### Phase 6C Testing
1. Authentication bypass attempts
2. Long-term data logging (7+ days)
3. Email notification delivery
4. Webhook reliability
5. Factory reset completeness

### Load Testing
- Concurrent web interface users (max 4-5)
- API request rate limits
- WebSocket message throughput
- Memory leak detection (run 7+ days)

---

## Risk Assessment

### Technical Risks

**Memory Exhaustion**:
- *Risk*: ESP32 RAM limitations with complex features
- *Mitigation*: Careful memory profiling, use PROGMEM, limit buffers

**Web Interface Performance**:
- *Risk*: Slow page loads, unresponsive UI
- *Mitigation*: GZIP compression, minimal JavaScript, lazy loading

**Configuration Corruption**:
- *Risk*: Invalid config causing boot failure
- *Mitigation*: Validation on load, safe mode fallback, config versioning

**Security Vulnerabilities**:
- *Risk*: Unauthorized access to device control
- *Mitigation*: Authentication, input validation, rate limiting

### Hardware Risks

**Flash Wear**:
- *Risk*: Frequent config writes degrading flash memory
- *Mitigation*: Write only on changes, use wear-leveling (built into ESP32)

**Network Reliability**:
- *Risk*: WiFi dropouts during critical operations
- *Mitigation*: Graceful degradation, local caching, retry logic

---

## Success Criteria

### Phase 6A
- ✅ Device configurable without recompilation
- ✅ Web interface accessible on local network
- ✅ Configuration persists across reboots
- ✅ AP mode allows initial setup

### Phase 6B
- ✅ Real-time status updates without page refresh
- ✅ Historical data visualization functional
- ✅ Mobile-friendly responsive interface
- ✅ OTA updates working reliably

### Phase 6C
- ✅ Authenticated access required
- ✅ 7+ days of data logged and retrievable
- ✅ Email notifications delivered
- ✅ Integration webhooks functional

---

## Next Steps

1. **Decision**: Choose implementation approach (Minimal, Feature-Complete, or Phased)
2. **Setup**: Add required libraries to platformio.ini
3. **Design**: Finalize configuration structure and API contracts
4. **Implement**: Start with ConfigManager foundation
5. **Test**: Validate each component before integration
6. **Document**: Update CLAUDE.md with Phase 6 details

---

## Questions for Decision Making

1. **Scope**: Minimal quick win or feature-complete professional system?
2. **Security**: Authentication required? (depends on network environment)
3. **Data Logging**: How much historical data needed? (impacts flash usage)
4. **Integration**: Which external systems to support? (IFTTT, Prometheus, webhooks?)
5. **UI Complexity**: Simple status page or rich dashboard with charts?

**Recommendation**: Start with **Phase 6A** (minimal approach) to prove the architecture, then evaluate whether 6B/6C features are needed based on actual usage patterns.
