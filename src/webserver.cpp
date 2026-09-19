/// src/webserver.cpp
#include "webserver.h"
#include <cstdlib>
#include <cmath>

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
      running(false),
      rebootPending(false),
      rebootRequestedAt(0) {
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

    this->server->on("/override", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleOverride(request);
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

bool PlantWebServer::parseLongParam(AsyncWebServerRequest* request, const char* name,
                                     long min, long max, long& out) {
    if (!request->hasParam(name, true)) {
        return false;
    }

    const String& value = request->getParam(name, true)->value();
    if (value.length() == 0) {
        return false;
    }

    char* end = nullptr;
    long parsed = strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        return false;  /// empty or trailing garbage
    }

    if (parsed < min || parsed > max) {
        return false;
    }

    out = parsed;
    return true;
}

bool PlantWebServer::parseFloatParam(AsyncWebServerRequest* request, const char* name,
                                      float min, float max, float& out) {
    if (!request->hasParam(name, true)) {
        return false;
    }

    const String& value = request->getParam(name, true)->value();
    if (value.length() == 0) {
        return false;
    }

    char* end = nullptr;
    float parsed = strtof(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0') {
        return false;
    }

    /// strtof accepts "nan", and NaN compares false against any bound, so
    /// it would pass the range check (and ConfigManager::validate) silently
    if (std::isnan(parsed) || parsed < min || parsed > max) {
        return false;
    }

    out = parsed;
    return true;
}

String PlantWebServer::htmlEscape(const String& value) {
    String escaped;
    escaped.reserve(value.length());
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

void PlantWebServer::sendMessagePage(AsyncWebServerRequest* request, int code, const char* title,
                                      const String& body, bool isError) {
    String color = isError ? "#dc3545" : "#28a745";
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>";
    html += title;
    html += "</title><style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;"
            "padding:20px;background:#f0f0f0;}h2{color:";
    html += color;
    html += ";}</style></head><body><h2>";
    html += title;
    html += "</h2><p>";
    html += body;
    html += "</p></body></html>";

    request->send(code, "text/html", html);
}

void PlantWebServer::applyConfig(const PlantLightConfig& config) {
    /// The one place that pushes a validated config into running components -
    /// saving to flash alone doesn't affect the live decision logic until
    /// the next reboot. Always applied even when a reboot follows shortly
    /// after; harmless and simpler than special-casing it away
    this->plantController->updateConfiguration(config.lightStartHour, config.lightEndHour,
        config.lightThresholdLux, config.hysteresisLux);
    this->relayController->setMinSwitchInterval(config.minSwitchIntervalMs);
}

void PlantWebServer::handleSave(AsyncWebServerRequest* request) {
    /// We validate a candidate copy so a rejected field never mutates the
    /// live configuration - this handler runs on the async_tcp task and
    /// must not touch controller/config state directly
    PlantLightConfig candidate = this->configManager->getConfig();

    long startHour = 0, endHour = 0, minSwitchIntervalSec = 0, timezone = 0;
    float threshold = 0, hysteresis = 0;

    if (!parseLongParam(request, "start_hour", 0, 23, startHour)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Invalid or missing field: start_hour (must be 0-23).", true);
        return;
    }
    if (!parseLongParam(request, "end_hour", 0, 23, endHour)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Invalid or missing field: end_hour (must be 0-23).", true);
        return;
    }
    if (!parseFloatParam(request, "threshold", 0.0f, 10000.0f, threshold)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Invalid or missing field: threshold (must be 0-10000).", true);
        return;
    }
    if (!parseFloatParam(request, "hysteresis", 0.0f, 100.0f, hysteresis)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Invalid or missing field: hysteresis (must be 0-100).", true);
        return;
    }
    /// Range-checked in seconds (1-600) *before* the caller multiplies by
    /// 1000 - a 32-bit long overflows above ~2.1e6 s, so checking the
    /// post-multiplication value would let huge inputs wrap around
    if (!parseLongParam(request, "min_switch_interval", 1, 600, minSwitchIntervalSec)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Invalid or missing field: min_switch_interval (must be 1-600 seconds).", true);
        return;
    }
    if (!parseLongParam(request, "timezone", -12, 14, timezone)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Invalid or missing field: timezone (must be -12 to +14).", true);
        return;
    }
    if (!request->hasParam("wifi_ssid", true) || !request->hasParam("wifi_pass", true)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Missing field: wifi_ssid or wifi_pass.", true);
        return;
    }

    String newSSID = request->getParam("wifi_ssid", true)->value();
    String newPassword = request->getParam("wifi_pass", true)->value();

    bool needsReboot = strcmp(candidate.wifiSSID, newSSID.c_str()) != 0 ||
                        strcmp(candidate.wifiPassword, newPassword.c_str()) != 0 ||
                        candidate.timezoneOffsetHours != timezone;

    candidate.lightStartHour = static_cast<uint8_t>(startHour);
    candidate.lightEndHour = static_cast<uint8_t>(endHour);
    candidate.lightThresholdLux = threshold;
    candidate.hysteresisLux = hysteresis;
    candidate.minSwitchIntervalMs = static_cast<uint32_t>(minSwitchIntervalSec) * 1000;
    candidate.timezoneOffsetHours = static_cast<int8_t>(timezone);
    strncpy(candidate.wifiSSID, newSSID.c_str(), sizeof(candidate.wifiSSID) - 1);
    candidate.wifiSSID[sizeof(candidate.wifiSSID) - 1] = '\0';
    strncpy(candidate.wifiPassword, newPassword.c_str(), sizeof(candidate.wifiPassword) - 1);
    candidate.wifiPassword[sizeof(candidate.wifiPassword) - 1] = '\0';

    if (!ConfigManager::validate(candidate)) {
        sendMessagePage(request, 400, "Invalid Configuration",
            "❌ Configuration validation failed. Check serial monitor for details.", true);
        return;
    }

    WebRequest queued{};
    queued.type = WebRequestType::SaveConfig;
    queued.config = candidate;
    queued.needsReboot = needsReboot;
    queued.request = request->pause();

    {
        std::lock_guard<std::mutex> lock(this->pendingMutex);
        this->pending.push_back(queued);
    }
}

void PlantWebServer::handleOverride(AsyncWebServerRequest* request) {
    ManualOverride mode;

    if (request->hasParam("mode", true)) {
        String value = request->getParam("mode", true)->value();
        if (value == "on") {
            mode = ManualOverride::ForceOn;
        } else if (value == "off") {
            mode = ManualOverride::ForceOff;
        } else if (value == "auto") {
            mode = ManualOverride::Auto;
        } else {
            sendMessagePage(request, 400, "Invalid Request",
                "❌ Invalid mode value (must be on, off, or auto).", true);
            return;
        }
    } else {
        sendMessagePage(request, 400, "Invalid Request", "❌ Missing field: mode.", true);
        return;
    }

    WebRequest queued{};
    queued.type = WebRequestType::SetOverride;
    queued.override = mode;
    queued.request = request->pause();

    {
        std::lock_guard<std::mutex> lock(this->pendingMutex);
        this->pending.push_back(queued);
    }
}

void PlantWebServer::processPendingRequests() {
    std::deque<WebRequest> toProcess;
    {
        std::lock_guard<std::mutex> lock(this->pendingMutex);
        toProcess.swap(this->pending);
    }

    for (WebRequest& req : toProcess) {
        if (req.type == WebRequestType::SaveConfig) {
            /// We keep the previous config so a failed flash write doesn't
            /// leave the in-memory copy diverged from what's persisted
            PlantLightConfig previous = this->configManager->getConfig();
            this->configManager->setConfig(req.config);

            if (!this->configManager->saveConfiguration()) {
                this->configManager->setConfig(previous);
                if (auto request = req.request.lock()) {
                    sendMessagePage(request.get(), 500, "Save Failed",
                        "❌ Could not save configuration to flash memory.", true);
                }
                continue;
            }

            applyConfig(req.config);

            String message = "✅ Settings saved successfully!";
            if (req.needsReboot) {
                message += "<br><br>⚠️  WiFi or timezone changed. Device will reboot in 3 seconds...";
                message += "<br><br><a href='/'>← Back to Dashboard</a>";
                if (auto request = req.request.lock()) {
                    sendMessagePage(request.get(), 200, "Settings Saved", message, false);
                }
                Serial.println("🔄 WebServer: Scheduling reboot in 3 seconds...");
                this->rebootPending = true;
                this->rebootRequestedAt = millis();
            } else {
                message += "<br><br>Changes applied immediately.";
                message += "<br><br><a href='/'>← Back to Dashboard</a>";
                if (auto request = req.request.lock()) {
                    sendMessagePage(request.get(), 200, "Settings Saved", message, false);
                }
            }
        } else {  /// SetOverride
            this->plantController->setManualOverride(req.override);
            if (auto request = req.request.lock()) {
                request->redirect("/");
            }
        }
    }

    /// 3s gives the response time to leave the TCP stack before the reset
    if (this->rebootPending && millis() - this->rebootRequestedAt >= 3000) {
        ESP.restart();
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
        .override-form {
            display: inline-block;
            width: 32%;
        }
        .override-active {
            background: #2c3e50 !important;
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
    html += generateOverrideSection();
    html += generateSettingsSection();

    html += R"(
</body>
</html>
)";

    return html;
}

String PlantWebServer::generateStatusSection() {
    /// Get current status from components
    bool relayOn = this->relayController->getRelayState();
    float lightLevel = this->lightSensor->getCurrentLux();
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

String PlantWebServer::generateOverrideSection() {
    ManualOverride mode = this->plantController->getManualOverride();

    String autoClass = (mode == ManualOverride::Auto) ? "override-active" : "";
    String onClass = (mode == ManualOverride::ForceOn) ? "override-active" : "";
    String offClass = (mode == ManualOverride::ForceOff) ? "override-active" : "";

    String modeLabel;
    switch (mode) {
        case ManualOverride::ForceOn: modeLabel = "🔒 Forced ON"; break;
        case ManualOverride::ForceOff: modeLabel = "🔒 Forced OFF"; break;
        default: modeLabel = "🤖 Automatic"; break;
    }

    String html = R"(
    <div class="container">
        <h2>🔘 Manual Override</h2>
        <p>Current mode: <strong>)";
    html += modeLabel;
    html += R"(</strong></p>
        <form action="/override" method="POST" class="override-form">
            <input type="hidden" name="mode" value="auto">
            <button type="submit" class=")";
    html += autoClass;
    html += R"(">🤖 Auto</button>
        </form>
        <form action="/override" method="POST" class="override-form">
            <input type="hidden" name="mode" value="on">
            <button type="submit" class=")";
    html += onClass;
    html += R"(">💡 Force ON</button>
        </form>
        <form action="/override" method="POST" class="override-form">
            <input type="hidden" name="mode" value="off">
            <button type="submit" class=")";
    html += offClass;
    html += R"(">🌙 Force OFF</button>
        </form>
        <div class="input-hint">Overrides the schedule and light sensor until set back to Auto - resets to Auto on reboot</div>
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
                <div class="input-hint">Lights turn on below threshold - h and off above threshold + h (0 = disabled)</div>
            </div>

            <div class="form-group">
                <label for="min_switch_interval">Anti-chatter Interval (seconds)</label>
                <input type="number" id="min_switch_interval" name="min_switch_interval" min="1" max="600" value=")";
    html += String(config.minSwitchIntervalMs / 1000);
    html += R"(" required>
                <div class="input-hint">Minimum time between relay switches, however sure the decision logic is</div>
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
    html += htmlEscape(String(config.wifiSSID));
    html += R"(" required>
            </div>

            <div class="form-group">
                <label for="wifi_pass">WiFi Password</label>
                <input type="password" id="wifi_pass" name="wifi_pass" maxlength="63" value=")";
    html += htmlEscape(String(config.wifiPassword));
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
    if (!this->timeManager->hasValidTime()) {
        return "---";
    }

    int hour = this->timeManager->getCurrentHour();
    int minute = this->timeManager->getCurrentMinute();
    int second = this->timeManager->getCurrentSecond();

    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hour, minute, second);
    return String(timeStr);
}
