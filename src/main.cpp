///
/// WiFi and Time Test Program
/// 
/// We test the WiFi connection and NTP time synchronization
/// components before integrating them with the plant light
/// control system. This verifies network connectivity and
/// time-based logic functionality.
///

#include <Arduino.h>
#include "wifimanager.h"
#include "timemanager.h"
#include "config.h"

WiFiManager* wifiManager;
TimeManager* timeManager;

void displaySystemStatus();
void displayTimeStatus();
void displayPlantLightScheduleStatus();
void displayWiFiStatus();

void setup() {
	/// We initialize serial communication for debugging
	Serial.begin(115200);
	while (!Serial) {
		delay(10);
	}
	
	Serial.println("\n=== Smart Plant Light Controller - WiFi & Time Test ===");
	Serial.println("Testing WiFi connection and NTP time synchronization");
	Serial.println();
	
	/// We initialize WiFi manager
	wifiManager = new WiFiManager(WIFI_SSID, WIFI_PASSWORD);
	wifiManager->begin();
	
	/// We wait for WiFi connection before starting time sync
	Serial.println("Waiting for WiFi connection...");
	unsigned long wifiStartTime = millis();
	const unsigned long wifiTimeout = 30000; /// 30 second timeout
	
	while (!wifiManager->isConnected() && millis() - wifiStartTime < wifiTimeout) {
		wifiManager->update();
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	
	if (wifiManager->isConnected()) {
		Serial.println("✓ WiFi connected successfully");
		
		/// We initialize time manager after WiFi is connected
		timeManager = new TimeManager(NTP_SERVER, TIMEZONE_OFFSET_HOURS);
		timeManager->begin();
	} else {
		Serial.println("✗ WiFi connection failed - continuing without time sync");
		Serial.println("Check your WiFi credentials in config.h");
	}
	
	/// We display the plant light schedule configuration
	Serial.println();
	Serial.println("━━━ Plant Light Schedule Configuration ━━━");
	Serial.print("Light ON time:  ");
	Serial.print(LIGHT_START_HOUR);
	Serial.println(":00");
	Serial.print("Light OFF time: ");
	Serial.print(LIGHT_END_HOUR);
	Serial.println(":00");
	Serial.print("Light threshold: ");
	Serial.print(LIGHT_THRESHOLD_LUX);
	Serial.println(" lux");
	Serial.println();
}

void loop() {
	static unsigned long lastStatusDisplay = 0;
	const unsigned long displayInterval = 10000; /// Display status every 10 seconds
	
	unsigned long currentTime = millis();
	
	/// We continuously update WiFi and time managers
	wifiManager->update();
	
	if (timeManager && wifiManager->isConnected()) {
		timeManager->update();
	}
	
	/// We display system status periodically
	if (currentTime - lastStatusDisplay >= displayInterval) {
		lastStatusDisplay = currentTime;
		displaySystemStatus();
	}
	
	/// We add a small delay to prevent overwhelming the system
	delay(1000);
}

void displaySystemStatus() {
	Serial.println("━━━ System Status ━━━");
	
	/// We display WiFi status
	displayWiFiStatus();
	Serial.println();
	
	/// We display time status if available
	if (timeManager) {
		displayTimeStatus();
	} else {
		Serial.println("⏰ Time: Not initialized (WiFi required)");
	}
	
	Serial.println();
}

void displayWiFiStatus() {
	Serial.print("📡 WiFi: ");
	
	WiFiStatus status = wifiManager->getStatus();
	switch (status) {
		case WiFiStatus::Connected:
			Serial.print("✓ CONNECTED");
			break;
		case WiFiStatus::Connecting:
			Serial.print("⏳ CONNECTING");
			break;
		case WiFiStatus::Disconnected:
			Serial.print("✗ DISCONNECTED");
			break;
		case WiFiStatus::Failed:
			Serial.print("✗ FAILED");
			break;
	}
	
	if (wifiManager->isConnected()) {
		Serial.print(" (");
		Serial.print(wifiManager->getLocalIP());
		Serial.print(", ");
		Serial.print(wifiManager->getSignalStrength());
		Serial.print(" dBm)");
	}
	
	Serial.println();
	Serial.print("   Connection attempts: ");
	Serial.println(wifiManager->getConnectionAttempts());
	
	if (!wifiManager->isConnected()) {
		Serial.print("   Time since last connection: ");
		unsigned long timeSince = wifiManager->getTimeSinceLastConnection();
		if (timeSince == ULONG_MAX) {
			Serial.println("Never connected");
		} else {
			Serial.print(timeSince / 1000);
			Serial.println(" seconds");
		}
	}
}

void displayTimeStatus() {
	Serial.print("⏰ Time: ");
	
	if (timeManager->hasValidTime()) {
		Serial.print("✓ SYNCHRONIZED - ");
		Serial.print(timeManager->getCurrentTimeString());
		Serial.print(" (");
		Serial.print(timeManager->getCurrentDateString());
		Serial.println(")");
		
		/// We display sync information
		Serial.print("   Last sync: ");
		unsigned long timeSinceSync = timeManager->getTimeSinceLastSync();
		Serial.print(timeSinceSync / 1000);
		Serial.print(" seconds ago, ");
		Serial.print(timeManager->getSyncCount());
		Serial.println(" total syncs");
		
		/// We test the time-based plant light logic
		displayPlantLightScheduleStatus();
		
	} else {
		Serial.println("✗ NOT SYNCHRONIZED");
		Serial.print("   Sync attempts: ");
		Serial.println(timeManager->getSyncCount());
		
		if (timeManager->needsSync()) {
			Serial.println("   Status: Needs sync");
		}
	}
}

void displayPlantLightScheduleStatus() {
	/// We test the plant light schedule logic
	bool inSchedule = timeManager->isTimeInRange(LIGHT_START_HOUR, LIGHT_END_HOUR);
	int currentHour = timeManager->getCurrentHour();
	
	Serial.print("   Schedule: ");
	if (inSchedule) {
		Serial.print("🌱 IN SCHEDULE (lights should be available");
		if (LIGHT_START_HOUR > LIGHT_END_HOUR) {
			Serial.print(" - overnight schedule");
		}
		Serial.println(")");
	} else {
		Serial.println("🌙 OUT OF SCHEDULE (lights should be off regardless of light level)");
	}
	
	Serial.print("   Current hour: ");
	Serial.print(currentHour);
	Serial.print(", Schedule: ");
	Serial.print(LIGHT_START_HOUR);
	Serial.print(":00 to ");
	Serial.print(LIGHT_END_HOUR);
	Serial.println(":00");
}

/// We clean up memory on program end
void cleanup() {
	if (timeManager) {
		delete timeManager;
		timeManager = nullptr;
	}
	
	if (wifiManager) {
		delete wifiManager;
		wifiManager = nullptr;
	}
}