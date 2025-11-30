///
/// Full Integration Test - Complete Smart Plant Light Controller
/// 
/// We integrate all components to create the complete smart plant
/// light control system. This combines WiFi, time synchronization,
/// light sensing, and relay control with intelligent decision logic.
///

#include <Arduino.h>
#include <Wire.h>
#include "wifimanager.h"
#include "timemanager.h"
#include "lightsensor.h"
#include "relaycontroller.h"
#include "plantcontroller.h"
#include "systemdiagnostics.h"
#include "configmanager.h"
#include "webserver.h"
#include "config.h"

/// Component instances
ConfigManager* configManager;
WiFiManager* wifiManager;
TimeManager* timeManager;
LightSensor* lightSensor;
RelayController* relayController;
PlantController* plantController;
SystemDiagnostics* diagnostics;
PlantWebServer* webServer;

void displaySystemStatus();
void displayTimeStatus();
void displayPlantLightScheduleStatus();
void displayWiFiStatus();
void displayControlStatus();
void initializeComponents();
void waitForSystemReady();
void displaySystemConfiguration();
void displayFullSystemStatus();
void displayConnectivityStatus();
void displaySensorStatus();
void displayRelayStatus();

void setup() {
	/// We initialize serial communication for debugging
	Serial.begin(115200);
	while (!Serial) {
		delay(10);
	}

	Serial.println("\n████████████████████████████████████████████████████████");
	Serial.println("███ Smart Plant Light Controller - Full Integration ███");
	Serial.println("████████████████████████████████████████████████████████");
	Serial.println();

	/// We initialize configuration manager first
	Serial.println("🔧 Initializing Configuration Manager...");
	configManager = new ConfigManager();
	configManager->begin();
	Serial.println();

	/// We initialize diagnostics to track startup
	Serial.println("🔧 Initializing System Diagnostics...");
	diagnostics = new SystemDiagnostics();
	diagnostics->begin();
	diagnostics->recordStartup();
	Serial.println();

	/// We initialize I2C for the light sensor
	Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
	Serial.print("I2C initialized - SDA: GPIO");
	Serial.print(I2C_SDA_PIN);
	Serial.print(", SCL: GPIO");
	Serial.println(I2C_SCL_PIN);

	/// We initialize all components in dependency order
	initializeComponents();
	
	/// We wait for essential components to be ready
	waitForSystemReady();

	/// We initialize the main plant controller with diagnostics
	plantController = new PlantController(wifiManager, timeManager, lightSensor, relayController, diagnostics);
	plantController->begin();

	/// We update plant controller with runtime configuration
	const PlantLightConfig& config = configManager->getConfig();
	plantController->updateConfiguration(config.lightStartHour, config.lightEndHour, config.lightThresholdLux, config.hysteresisLux);

	/// We initialize web server if WiFi is connected
	if (wifiManager->isConnected()) {
		Serial.println("🌐 Starting Web Server...");
		webServer = new PlantWebServer(configManager, plantController, lightSensor, timeManager, wifiManager, relayController);
		webServer->begin();
		Serial.print("✅ Web interface available at http://");
		Serial.println(wifiManager->getLocalIP());
	} else {
		webServer = nullptr;
	}

	Serial.println();
	Serial.println("🌱 Smart Plant Light Controller is now ACTIVE!");
	Serial.println("The system will automatically control your plant lights based on:");
	Serial.println("  📅 Time schedule AND 💡 ambient light levels");
	Serial.println();
	displaySystemConfiguration();
	Serial.println();
}

void loop() {
	static unsigned long lastStatusDisplay = 0;
	static unsigned long lastSensorUpdate = 0;
	const unsigned long displayInterval = 15000;  /// Status every 15 seconds
	const unsigned long sensorInterval = 2000;    /// Sensor updates every 2 seconds
	
	unsigned long currentTime = millis();
	
	/// We continuously update all components
	wifiManager->update();
	
	if (wifiManager->isConnected()) {
		timeManager->update();
	}
	
	/// We update sensor readings regularly
	if (currentTime - lastSensorUpdate >= sensorInterval) {
		lastSensorUpdate = currentTime;
		if (lightSensor->updateReading()) {
			/// We update sensor stability metrics
			diagnostics->updateSensorMetric(lightSensor->getCurrentLux());
		} else {
			Serial.println("⚠ Light sensor reading failed");
		}
	}

	/// We update WiFi metrics if connected
	if (wifiManager->isConnected()) {
		diagnostics->updateWiFiMetric(wifiManager->getSignalStrength());
	}

	/// We run the main plant control logic
	plantController->update();
	
	/// We display comprehensive status periodically
	if (currentTime - lastStatusDisplay >= displayInterval) {
		lastStatusDisplay = currentTime;
		displayFullSystemStatus();
	}
	
	/// We add a small delay to prevent system overload
	delay(500);
}

void initializeComponents() {
	Serial.println("🔧 Initializing system components...");

	/// We get configuration from ConfigManager
	const PlantLightConfig& config = configManager->getConfig();

	/// We initialize WiFi manager with runtime credentials
	Serial.println("  📡 WiFi Manager...");
	wifiManager = new WiFiManager(config.wifiSSID, config.wifiPassword);
	wifiManager->begin();
	
	/// We initialize relay controller (must be first for safety)
	Serial.println("  🔌 Relay Controller...");
	relayController = new RelayController(RELAY_PIN);
	relayController->begin();
	
	/// We initialize light sensor
	Serial.println("  💡 Light Sensor...");
	lightSensor = new LightSensor();
	if (!lightSensor->begin()) {
		Serial.println("  ✗ Light sensor initialization failed!");
		while (true) { delay(1000); }
	}
	
	Serial.println("✓ All components initialized");
}

void waitForSystemReady() {
	Serial.println("⏳ Waiting for system to be ready...");
	
	/// We wait for WiFi connection
	Serial.println("  📡 Waiting for WiFi connection...");
	unsigned long wifiStartTime = millis();
	const unsigned long wifiTimeout = 60000; /// 60 second timeout
	
	while (!wifiManager->isConnected() && millis() - wifiStartTime < wifiTimeout) {
		wifiManager->update();
		delay(1000);
		Serial.print(".");
	}
	Serial.println();
	
	if (wifiManager->isConnected()) {
		Serial.println("  ✓ WiFi connected");

		/// We get configuration from ConfigManager
		const PlantLightConfig& config = configManager->getConfig();

		/// We initialize time manager after WiFi is ready with runtime timezone
		Serial.println("  ⏰ Time Manager...");
		timeManager = new TimeManager(NTP_SERVER, config.timezoneOffsetHours);
		timeManager->begin();
		
		/// We wait for initial time sync
		Serial.println("  ⏰ Waiting for time synchronization...");
		unsigned long timeStartTime = millis();
		const unsigned long timeTimeout = 30000; /// 30 second timeout
		
		while (!timeManager->hasValidTime() && millis() - timeStartTime < timeTimeout) {
			timeManager->update();
			delay(1000);
			Serial.print(".");
		}
		Serial.println();
		
		if (timeManager->hasValidTime()) {
			Serial.println("  ✓ Time synchronized");
		} else {
			Serial.println("  ⚠ Time sync failed - continuing with limited functionality");
		}
	} else {
		Serial.println("  ⚠ WiFi connection failed - continuing without time sync");
		timeManager = nullptr;
	}
	
	/// We take initial sensor readings
	Serial.println("  💡 Taking initial sensor readings...");
	for (int i = 0; i < 5; i++) {
		lightSensor->updateReading();
		delay(500);
	}
	
	Serial.println("✓ System ready for operation");
}

void displaySystemConfiguration() {
	const PlantLightConfig& config = configManager->getConfig();

	Serial.println("━━━ System Configuration ━━━");
	Serial.print("📅 Schedule: ");
	Serial.print(config.lightStartHour);
	Serial.print(":00 - ");
	Serial.print(config.lightEndHour);
	Serial.print(":00 ");
	if (config.lightStartHour > config.lightEndHour) {
		Serial.println("(overnight schedule)");
	} else {
		Serial.println("(daytime schedule)");
	}

	Serial.print("💡 Light threshold: ");
	Serial.print(config.lightThresholdLux);
	Serial.println(" lux");

	Serial.print("⚡ Hysteresis: ");
	Serial.print(config.hysteresisLux);
	Serial.println(" lux");

	Serial.print("🌐 Timezone: UTC");
	Serial.print(config.timezoneOffsetHours >= 0 ? "+" : "");
	Serial.println(config.timezoneOffsetHours);

	Serial.print("🔄 Check interval: ");
	Serial.print(CHECK_INTERVAL_MS / 1000);
	Serial.println(" seconds");

	Serial.print("🔌 Relay pin: GPIO");
	Serial.println(RELAY_PIN);
}

void displayFullSystemStatus() {
	Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	Serial.println("                 🌱 SYSTEM STATUS 🌱");
	Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	
	/// We display connectivity status
	displayConnectivityStatus();
	Serial.println();
	
	/// We display time status
	displayTimeStatus();
	Serial.println();
	
	/// We display sensor status
	displaySensorStatus();
	Serial.println();
	
	/// We display relay status
	displayRelayStatus();
	Serial.println();
	
	/// We display control logic status
	displayControlStatus();
	Serial.println();

	/// We display diagnostics
	diagnostics->displayReport();

	Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
	Serial.println();
}

void displayConnectivityStatus() {
	Serial.print("📡 WiFi: ");
	if (wifiManager->isConnected()) {
		Serial.print("✅ CONNECTED (");
		Serial.print(wifiManager->getLocalIP());
		Serial.print(", ");
		Serial.print(wifiManager->getSignalStrength());
		Serial.println(" dBm)");
	} else {
		Serial.println("❌ DISCONNECTED");
	}
}

void displayTimeStatus() {
	Serial.print("⏰ Time: ");
	if (timeManager && timeManager->hasValidTime()) {
		Serial.print("✅ ");
		Serial.print(timeManager->getCurrentTimeString());
		Serial.print(" (synced ");
		Serial.print(timeManager->getTimeSinceLastSync() / 1000);
		Serial.println("s ago)");
	} else {
		Serial.println("❌ NO VALID TIME");
	}
}

void displaySensorStatus() {
	const PlantLightConfig& config = configManager->getConfig();

	Serial.print("💡 Light: ");
	if (lightSensor->isSensorHealthy()) {
		float lux = lightSensor->getCurrentLux();
		Serial.print("✅ ");
		Serial.print(lux, 1);
		Serial.print(" lux (");
		Serial.print(lux < config.lightThresholdLux ? "DARK" : "BRIGHT");
		Serial.println(")");
	} else {
		Serial.println("❌ SENSOR FAILURE");
	}
}

void displayRelayStatus() {
	bool relayOn = relayController->getRelayState();
	Serial.print("🔌 Relay: ");
	Serial.print(relayOn ? "✅ ON" : "⭕ OFF");
	Serial.print(" (");
	Serial.print(plantController->getRelayChanges());
	Serial.println(" changes total)");
}

void displayControlStatus() {
	Serial.print("🤖 Control: ");
	if (plantController->areAllComponentsHealthy()) {
		Serial.print("✅ ACTIVE - ");
		
		/// We show the current logic decision
		ControlDecision decision = plantController->getLastDecision();
		ControlReason reason = plantController->getLastReason();
		
		switch (decision) {
			case ControlDecision::TurnOn:
				Serial.print("🌙 LIGHTS ON");
				break;
			case ControlDecision::TurnOff:
				Serial.print("☀️ LIGHTS OFF");
				break;
			case ControlDecision::KeepCurrent:
				Serial.print("↔️ NO CHANGE");
				break;
			case ControlDecision::WaitForData:
				Serial.print("⏳ WAITING");
				break;
		}
		
		Serial.print(" (");
		switch (reason) {
			case ControlReason::OutOfSchedule:
				Serial.print("out of schedule");
				break;
			case ControlReason::InScheduleDark:
				Serial.print("in schedule + dark");
				break;
			case ControlReason::InScheduleBright:
				Serial.print("in schedule + bright");
				break;
			default:
				Serial.print("system issue");
				break;
		}
		Serial.println(")");
		
		Serial.print("    Decisions made: ");
		Serial.println(plantController->getDecisionCount());
		
	} else {
		Serial.println("❌ DEGRADED (missing data)");
	}
}

/// We clean up memory on program end
void cleanup() {
	if (diagnostics) {
		diagnostics->clearCrashMarker();  /// Mark clean shutdown
		delete diagnostics;
		diagnostics = nullptr;
	}
	if (webServer) { delete webServer; webServer = nullptr; }
	if (plantController) { delete plantController; plantController = nullptr; }
	if (relayController) { delete relayController; relayController = nullptr; }
	if (lightSensor) { delete lightSensor; lightSensor = nullptr; }
	if (timeManager) { delete timeManager; timeManager = nullptr; }
	if (wifiManager) { delete wifiManager; wifiManager = nullptr; }
	if (configManager) { delete configManager; configManager = nullptr; }
}