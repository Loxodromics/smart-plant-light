/// include/webserver.h
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <deque>
#include <mutex>
#include "configmanager.h"
#include "plantcontroller.h"
#include "lightsensor.h"
#include "timemanager.h"
#include "wifimanager.h"
#include "relaycontroller.h"

/// The type of a queued, already-validated web request waiting to be
/// applied on the loop task
enum class WebRequestType {
    SaveConfig,
    SetOverride
};

/// A parsed and validated request handed off from the async_tcp task to
/// the loop task. `request` is a weak_ptr (ESPAsyncWebServer's request
/// continuation API) - the client may disconnect while this sits in the
/// queue, so it must be lock()ed before use
struct WebRequest {
    WebRequestType type;
    PlantLightConfig config;       /// SaveConfig: full validated candidate
    bool needsReboot;              /// SaveConfig: WiFi credentials changed
    ManualOverride override;       /// SetOverride
    AsyncWebServerRequestPtr request;
};

/// Simple web server for plant light configuration and monitoring
/// We provide a single-page interface showing current status and
/// allowing users to modify settings without recompiling
///
/// Handlers run on the async_tcp FreeRTOS task, not the loop task, so they
/// only parse, validate, and enqueue a WebRequest here; processPendingRequests()
/// (called from loop()) is the only place that touches controller/config state,
/// blocks (e.g. for a reboot delay), or sends the real response. Any future
/// endpoint follows the same pattern.
class PlantWebServer {
public:
    PlantWebServer(ConfigManager* configMgr,
                   PlantController* plantCtrl,
                   LightSensor* lightSensor,
                   TimeManager* timeMgr,
                   WiFiManager* wifiMgr,
                   RelayController* relayCtrl);
    ~PlantWebServer();

    /// Start the web server on port 80
    void begin();

    /// Stop the web server
    void stop();

    /// Check if server is running
    [[nodiscard]] bool isRunning() const;

    /// Apply queued requests, send their real responses, and fire a
    /// pending reboot once it's due - called from loop() on the loop task
    void processPendingRequests();

private:
    AsyncWebServer* server;
    ConfigManager* configManager;
    PlantController* plantController;
    LightSensor* lightSensor;
    TimeManager* timeManager;
    WiFiManager* wifiManager;
    RelayController* relayController;

    bool running;

    std::deque<WebRequest> pending;
    std::mutex pendingMutex;
    bool rebootPending;
    unsigned long rebootRequestedAt;

    /// HTTP request handlers - run on the async_tcp task; parse, validate,
    /// and enqueue only, never touch controller/config state directly
    void handleRoot(AsyncWebServerRequest* request);
    void handleSave(AsyncWebServerRequest* request);
    void handleOverride(AsyncWebServerRequest* request);

    /// Parses a decimal long from a form field, rejecting empty values and
    /// trailing garbage, and range-checks it before the caller narrows
    [[nodiscard]] static bool parseLongParam(AsyncWebServerRequest* request, const char* name,
                                              long min, long max, long& out);
    [[nodiscard]] static bool parseFloatParam(AsyncWebServerRequest* request, const char* name,
                                               float min, float max, float& out);

    /// Escapes & < > " ' for safe interpolation into HTML attributes/text
    [[nodiscard]] static String htmlEscape(const String& value);

    /// Renders one of the small "result" pages shared by handleSave outcomes
    void sendMessagePage(AsyncWebServerRequest* request, int code, const char* title,
                          const String& body, bool isError);

    /// The one place that pushes a validated config into running components
    void applyConfig(const PlantLightConfig& config);

    /// HTML page generation
    String generateHTML();
    String generateStatusSection();
    String generateOverrideSection();
    String generateSettingsSection();

    /// Helper to get current time as string
    String getCurrentTimeString();
};

#endif
