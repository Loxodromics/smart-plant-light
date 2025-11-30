/// include/webserver.h
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include "configmanager.h"
#include "plantcontroller.h"
#include "lightsensor.h"
#include "timemanager.h"
#include "wifimanager.h"
#include "relaycontroller.h"

/// Simple web server for plant light configuration and monitoring
/// We provide a single-page interface showing current status and
/// allowing users to modify settings without recompiling
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

private:
    AsyncWebServer* server;
    ConfigManager* configManager;
    PlantController* plantController;
    LightSensor* lightSensor;
    TimeManager* timeManager;
    WiFiManager* wifiManager;
    RelayController* relayController;

    bool running;

    /// HTTP request handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleSave(AsyncWebServerRequest* request);

    /// HTML page generation
    String generateHTML();
    String generateStatusSection();
    String generateSettingsSection();

    /// Helper to get current time as string
    String getCurrentTimeString();
};

#endif
