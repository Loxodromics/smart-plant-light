/// src/webserver.cpp
#include "webserver.h"

PlantWebServer::PlantWebServer(ConfigManager* configMgr,
                               PlantController* plantCtrl,
                               LightSensor* lightSensor,
                               TimeManager* timeMgr,
                               WiFiManager* wifiMgr,
                               RelayController* relayCtrl)
    : server(nullptr),
      configManager(configMgr),
      plantController(plantCtrl),
      lightSensor(lightSensor),
      timeManager(timeMgr),
      wifiManager(wifiMgr),
      relayController(relayCtrl),
      running(false) {
}

PlantWebServer::~PlantWebServer() {
    stop();
}

void PlantWebServer::begin() {
    if (this->running) {
        Serial.println("⚠️  WebServer: Already running");
        return;
    }

    Serial.println("🌐 WebServer: Starting...");

    /// Create server instance on port 80
    this->server = new AsyncWebServer(80);

    /// Setup routes
    this->server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleRoot(request);
    });

    this->server->on("/save", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleSave(request);
    });

    /// Start server
    this->server->begin();
    this->running = true;

    Serial.println("✅ WebServer: Started on port 80");
}

void PlantWebServer::stop() {
    if (!this->running || this->server == nullptr) {
        return;
    }

    Serial.println("🛑 WebServer: Stopping...");
    this->server->end();
    delete this->server;
    this->server = nullptr;
    this->running = false;
}

bool PlantWebServer::isRunning() const {
    return this->running;
}

void PlantWebServer::handleRoot(AsyncWebServerRequest* request) {
    /// Generate and send the HTML page
    String html = generateHTML();
    request->send(200, "text/html", html);
}

void PlantWebServer::handleSave(AsyncWebServerRequest* request) {
    Serial.println("💾 WebServer: Processing configuration update...");

    bool needsReboot = false;

    /// Extract form parameters
    if (request->hasParam("wifi_ssid", true) && request->hasParam("wifi_pass", true)) {
        String newSSID = request->getParam("wifi_ssid", true)->value();
        String newPassword = request->getParam("wifi_pass", true)->value();

        /// Check if WiFi credentials changed (requires reboot)
        const PlantLightConfig& currentConfig = this->configManager->getConfig();
        if (strcmp(currentConfig.wifiSSID, newSSID.c_str()) != 0 ||
            strcmp(currentConfig.wifiPassword, newPassword.c_str()) != 0) {
            needsReboot = true;
        }

        this->configManager->setWiFiCredentials(newSSID.c_str(), newPassword.c_str());
        Serial.printf("  WiFi SSID: %s\n", newSSID.c_str());
    }

    if (request->hasParam("start_hour", true)) {
        uint8_t startHour = request->getParam("start_hour", true)->value().toInt();
        uint8_t endHour = this->configManager->getConfig().lightEndHour;

        if (request->hasParam("end_hour", true)) {
            endHour = request->getParam("end_hour", true)->value().toInt();
        }

        this->configManager->setSchedule(startHour, endHour);
        Serial.printf("  Schedule: %02d:00 - %02d:00\n", startHour, endHour);
    }

    if (request->hasParam("threshold", true)) {
        float threshold = request->getParam("threshold", true)->value().toFloat();
        this->configManager->setLightThreshold(threshold);
        Serial.printf("  Threshold: %.1f lux\n", threshold);
    }

    if (request->hasParam("hysteresis", true)) {
        float hysteresis = request->getParam("hysteresis", true)->value().toFloat();
        this->configManager->setHysteresis(hysteresis);
        Serial.printf("  Hysteresis: %.1f lux\n", hysteresis);
    }

    if (request->hasParam("timezone", true)) {
        int8_t timezone = request->getParam("timezone", true)->value().toInt();
        this->configManager->setTimezone(timezone);
        Serial.printf("  Timezone: UTC%+d\n", timezone);
        needsReboot = true;  /// Timezone change requires NTP resync
    }

    /// Validate and save configuration
    if (this->configManager->isValid()) {
        if (this->configManager->saveConfiguration()) {
            String message = "✅ Settings saved successfully!";
            if (needsReboot) {
                message += "<br><br>⚠️  WiFi or timezone changed. Device will reboot in 3 seconds...";
                message += "<br><br><a href='/'>← Back to Dashboard</a>";

                /// Send response
                request->send(200, "text/html",
                    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Settings Saved</title>"
                    "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;"
                    "padding:20px;background:#f0f0f0;}h2{color:#28a745;}</style></head>"
                    "<body><h2>Settings Saved</h2><p>" + message + "</p></body></html>");

                /// Schedule reboot
                Serial.println("🔄 WebServer: Scheduling reboot in 3 seconds...");
                delay(3000);
                ESP.restart();
            } else {
                message += "<br><br>Changes applied immediately.";
                message += "<br><br><a href='/'>← Back to Dashboard</a>";

                request->send(200, "text/html",
                    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Settings Saved</title>"
                    "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;"
                    "padding:20px;background:#f0f0f0;}h2{color:#28a745;}</style></head>"
                    "<body><h2>Settings Saved</h2><p>" + message + "</p></body></html>");
            }
        } else {
            request->send(500, "text/html",
                "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Save Failed</title>"
                "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;"
                "padding:20px;background:#f0f0f0;}h2{color:#dc3545;}</style></head>"
                "<body><h2>Save Failed</h2><p>❌ Could not save configuration to flash memory.</p>"
                "<p><a href='/'>← Back to Dashboard</a></p></body></html>");
        }
    } else {
        request->send(400, "text/html",
            "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Invalid Configuration</title>"
            "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;"
            "padding:20px;background:#f0f0f0;}h2{color:#dc3545;}</style></head>"
            "<body><h2>Invalid Configuration</h2><p>❌ Configuration validation failed. Check serial monitor for details.</p>"
            "<p><a href='/'>← Back to Dashboard</a></p></body></html>");
    }
}

String PlantWebServer::generateHTML() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="refresh" content="5">
    <title>🌱 Plant Light Controller</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            padding: 20px;
            margin-bottom: 20px;
        }
        h1 {
            color: #2c3e50;
            margin-top: 0;
            border-bottom: 3px solid #27ae60;
            padding-bottom: 10px;
        }
        h2 {
            color: #34495e;
            margin-top: 0;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .status-item {
            background: #ecf0f1;
            padding: 15px;
            border-radius: 6px;
            text-align: center;
        }
        .status-label {
            font-size: 14px;
            color: #7f8c8d;
            margin-bottom: 5px;
        }
        .status-value {
            font-size: 24px;
            font-weight: bold;
            color: #2c3e50;
        }
        .relay-on {
            background: #d4edda;
            color: #155724;
        }
        .relay-off {
            background: #f8d7da;
            color: #721c24;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #34495e;
            font-weight: bold;
        }
        input[type="text"],
        input[type="password"],
        input[type="number"] {
            width: 100%;
            padding: 8px;
            border: 1px solid #bdc3c7;
            border-radius: 4px;
            box-sizing: border-box;
            font-size: 14px;
        }
        .input-hint {
            font-size: 12px;
            color: #7f8c8d;
            margin-top: 3px;
        }
        button {
            background: #27ae60;
            color: white;
            padding: 12px 24px;
            border: none;
            border-radius: 4px;
            font-size: 16px;
            cursor: pointer;
            width: 100%;
        }
        button:hover {
            background: #229954;
        }
        .warning {
            background: #fff3cd;
            border: 1px solid #ffc107;
            padding: 10px;
            border-radius: 4px;
            color: #856404;
            margin-top: 10px;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <h1>🌱 Plant Light Controller</h1>
)";

    html += generateStatusSection();
    html += generateSettingsSection();

    html += R"(
</body>
</html>
)";

    return html;
}

String PlantWebServer::generateStatusSection() {
    /// Get current status from components
    bool relayOn = this->relayController->isRelayOn();
    float lightLevel = this->lightSensor->getAverageLux();
    String currentTime = getCurrentTimeString();
    int wifiSignal = this->wifiManager->getSignalStrength();

    String relayStatus = relayOn ? "ON 💡" : "OFF 🌙";
    String relayClass = relayOn ? "relay-on" : "relay-off";

    String html = R"(
    <div class="container">
        <h2>📊 Status</h2>
        <div class="status-grid">
            <div class="status-item )";
    html += relayClass;
    html += R"(">
                <div class="status-label">Relay State</div>
                <div class="status-value">)";
    html += relayStatus;
    html += R"(</div>
            </div>
            <div class="status-item">
                <div class="status-label">☀️ Light Level</div>
                <div class="status-value">)";
    html += String(lightLevel, 1);
    html += R"( lux</div>
            </div>
            <div class="status-item">
                <div class="status-label">🕐 Current Time</div>
                <div class="status-value">)";
    html += currentTime;
    html += R"(</div>
            </div>
            <div class="status-item">
                <div class="status-label">📶 WiFi Signal</div>
                <div class="status-value">)";
    html += String(wifiSignal);
    html += R"( dBm</div>
            </div>
        </div>
    </div>
)";

    return html;
}

String PlantWebServer::generateSettingsSection() {
    const PlantLightConfig& config = this->configManager->getConfig();

    String html = R"(
    <div class="container">
        <h2>⚙️ Settings</h2>
        <form action="/save" method="POST">
            <div class="form-group">
                <label for="start_hour">Start Hour (0-23)</label>
                <input type="number" id="start_hour" name="start_hour" min="0" max="23" value=")";
    html += String(config.lightStartHour);
    html += R"(" required>
                <div class="input-hint">Hour when lights can turn on</div>
            </div>

            <div class="form-group">
                <label for="end_hour">End Hour (0-23)</label>
                <input type="number" id="end_hour" name="end_hour" min="0" max="23" value=")";
    html += String(config.lightEndHour);
    html += R"(" required>
                <div class="input-hint">Hour when lights must turn off</div>
            </div>

            <div class="form-group">
                <label for="threshold">Light Threshold (lux)</label>
                <input type="number" id="threshold" name="threshold" min="0" max="10000" step="0.1" value=")";
    html += String(config.lightThresholdLux, 1);
    html += R"(" required>
                <div class="input-hint">Turn lights on when below this value</div>
            </div>

            <div class="form-group">
                <label for="hysteresis">Hysteresis (lux)</label>
                <input type="number" id="hysteresis" name="hysteresis" min="0" max="100" step="0.1" value=")";
    html += String(config.hysteresisLux, 1);
    html += R"(" required>
                <div class="input-hint">Dead band to prevent rapid switching (0 = disabled)</div>
            </div>

            <div class="form-group">
                <label for="timezone">Timezone Offset (UTC)</label>
                <input type="number" id="timezone" name="timezone" min="-12" max="14" value=")";
    html += String(config.timezoneOffsetHours);
    html += R"(" required>
                <div class="input-hint">Hours offset from UTC (e.g., Berlin = +1)</div>
            </div>

            <div class="form-group">
                <label for="wifi_ssid">WiFi SSID</label>
                <input type="text" id="wifi_ssid" name="wifi_ssid" maxlength="31" value=")";
    html += String(config.wifiSSID);
    html += R"(" required>
            </div>

            <div class="form-group">
                <label for="wifi_pass">WiFi Password</label>
                <input type="password" id="wifi_pass" name="wifi_pass" maxlength="63" value=")";
    html += String(config.wifiPassword);
    html += R"(" required>
            </div>

            <button type="submit">💾 Save Settings</button>

            <div class="warning">
                ⚠️ Changing WiFi or timezone will reboot the device
            </div>
        </form>
    </div>
)";

    return html;
}

String PlantWebServer::getCurrentTimeString() {
    if (!this->timeManager->isTimeValid()) {
        return "---";
    }

    int hour = this->timeManager->getCurrentHour();
    int minute = this->timeManager->getCurrentMinute();
    int second = this->timeManager->getCurrentSecond();

    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hour, minute, second);
    return String(timeStr);
}
