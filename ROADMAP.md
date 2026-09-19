# Roadmap

This is a backlog of known gaps and feature ideas, not a committed plan or
timeline - pick from it as needed. See the [README](README.md) for what's
already built.

## Known Gaps

- **Limited automated tests**: the decision logic (`src/controllogic.cpp`)
  has host-side Unity tests (`pio test -e native`), but everything that
  depends on Arduino/ESP32 headers (config validation, schedule/time handling,
  web request parsing) is still only verified on hardware.
- **No web UI authentication**: `PlantWebServer` has no auth and echoes the
  current WiFi password into the settings form. Treat it as trusted-LAN-only
  until this is addressed - especially before adding any remote-reachable
  integration (MQTT bridge, cloud logging, etc.).

## Feature Ideas

**MQTT Support**:
- Talk to a dedicated webserver or integration with Home Assistant, OpenHAB

**Smartphone App**:
- Dedicated mobile application

**Weather Integration**:
- Adjust based on weather forecasts

**OTA Firmware Updates**:
- Upload new firmware via web interface
- Automatic backup of current firmware
- Rollback capability on failed update

**WiFi AP Fallback Mode**:
- If WiFi connection fails or no credentials are configured, the device
  starts as an access point (`PlantLight-Setup`) with a captive portal
  redirecting to a setup page
- Avoids needing a serial connection for initial or changed WiFi setup

**mDNS Support**:
- Access device via `plantlight.local` instead of IP address
- Automatic discovery on network

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
- Store hourly readings to SPIFFS/LittleFS, circular buffer (7 days retention)
- Export logs as CSV
- Optionally forward to a cloud service for longer retention and historical analysis

**Web UI Authentication**:
- Basic Auth or an API token for the web interface and any future API
- Directly addresses the "no web UI authentication" gap above

**JSON REST API**:
- `GET /api/status`, `GET`/`POST /api/config` for programmatic access
  (Home Assistant, scripts) - the current web UI is server-rendered HTML
  forms only

**Factory Reset**:
- Restore `config.h` defaults via a web UI button, without needing to erase
  flash manually
