///
/// Combined Test Program - Light Sensor Controls Relay
/// 
/// We integrate the light sensor and relay controller to create
/// a basic automatic plant light system. This tests the core
/// functionality before adding WiFi and time-based controls.
///

#include <Arduino.h>
#include <Wire.h>
#include "lightsensor.h"
#include "relaycontroller.h"
#include "config.h"

LightSensor* lightSensor;
RelayController* relayController;

void setup() {
	/// We initialize serial communication for debugging output
	Serial.begin(115200);
	while (!Serial) {
		delay(10);
	}
	
	Serial.println("\n=== Smart Plant Light Controller - Combined Test ===");
	Serial.println("Testing automatic light control based on ambient light sensor");
	Serial.println();
	
	/// We initialize I2C communication for the light sensor
	Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
	Serial.print("I2C initialized - SDA: GPIO");
	Serial.print(I2C_SDA_PIN);
	Serial.print(", SCL: GPIO");
	Serial.println(I2C_SCL_PIN);
	
	/// We initialize the light sensor
	lightSensor = new LightSensor();
	if (!lightSensor->begin()) {
		Serial.println("✗ Failed to initialize light sensor - stopping");
		while (true) { delay(1000); }
	}
	Serial.println("✓ Light sensor initialized");
	
	/// We initialize the relay controller
	relayController = new RelayController(RELAY_PIN);
	relayController->begin();
	Serial.println("✓ Relay controller initialized");
	
	/// We display the configuration
	Serial.print("Light threshold: ");
	Serial.print(LIGHT_THRESHOLD_LUX);
	Serial.println(" lux");
	Serial.print("Sensor averaging: ");
	Serial.print(SENSOR_SAMPLES);
	Serial.println(" samples");
	Serial.print("Relay pin: GPIO");
	Serial.println(RELAY_PIN);
	Serial.print("Safety interval: ");
	Serial.print(MIN_SWITCH_INTERVAL_MS);
	Serial.println("ms");
	
	Serial.println();
	Serial.println("🚀 Starting automatic light control...");
	Serial.println("Try covering/uncovering the sensor to test switching");
	Serial.println();
}

void displaySystemStatus() {
	Serial.println("━━━ System Status ━━━");
	
	/// We display light sensor information
	float currentLux = lightSensor->getCurrentLux();
	float rawLux = lightSensor->getLastRawLux();
	bool belowThreshold = lightSensor->isBelowThreshold(LIGHT_THRESHOLD_LUX);
	
	Serial.print("💡 Light: ");
	Serial.print(currentLux, 1);
	Serial.print(" lux (raw: ");
	Serial.print(rawLux, 1);
	Serial.print(") - ");
	Serial.println(belowThreshold ? "🌙 DARK" : "☀️ BRIGHT");
	
	/// We display relay information
	bool relayState = relayController->getRelayState();
	unsigned long timeSinceSwitch = relayController->getTimeSinceLastSwitch();
	bool canSwitch = relayController->canSwitchRelay();
	
	Serial.print("🔌 Relay: ");
	Serial.print(relayState ? "ON" : "OFF");
	Serial.print(" (last switch: ");
	Serial.print(timeSinceSwitch);
	Serial.print("ms ago, can switch: ");
	Serial.print(canSwitch ? "YES" : "NO");
	Serial.println(")");
	
	/// We display decision logic
	Serial.print("🤖 Logic: ");
	if (belowThreshold && !relayState) {
		Serial.println("Should turn ON (dark + relay off)");
	} else if (!belowThreshold && relayState) {
		Serial.println("Should turn OFF (bright + relay on)");
	} else if (belowThreshold && relayState) {
		Serial.println("Correct state (dark + relay on)");
	} else {
		Serial.println("Correct state (bright + relay off)");
	}
	
	/// We display sensor health
	Serial.print("📊 Health: ");
	Serial.print("Sensor ");
	Serial.print(lightSensor->isSensorHealthy() ? "OK" : "FAIL");
	Serial.print(", ");
	Serial.print(lightSensor->getReadingCount());
	Serial.println(" total readings");
	
	Serial.println();
}

void loop() {
	static unsigned long lastSensorUpdate = 0;
	static unsigned long lastControlUpdate = 0;
	static unsigned long lastStatusDisplay = 0;
	
	const unsigned long sensorInterval = 1000;    /// We read sensor every second
	const unsigned long controlInterval = 2000;   /// We check control every 2 seconds
	const unsigned long displayInterval = 5000;   /// We display status every 5 seconds
	
	unsigned long currentTime = millis();
	
	/// We update sensor readings regularly
	if (currentTime - lastSensorUpdate >= sensorInterval) {
		lastSensorUpdate = currentTime;
		
		if (!lightSensor->updateReading()) {
			Serial.println("⚠ Sensor reading failed");
		}
	}
	
	/// We check if we need to change relay state
	if (currentTime - lastControlUpdate >= controlInterval) {
		lastControlUpdate = currentTime;
		
		/// We only proceed if sensor is healthy
		if (!lightSensor->isSensorHealthy()) {
			Serial.println("⚠ Sensor unhealthy - keeping current relay state");
			return;
		}
		
		/// We determine what the relay state should be
		bool shouldTurnOn = lightSensor->isBelowThreshold(LIGHT_THRESHOLD_LUX);
		bool currentState = relayController->getRelayState();
		
		/// We only attempt to change if needed
		if (shouldTurnOn != currentState) {
			Serial.print("💡 Light control decision: ");
			Serial.print(shouldTurnOn ? "Turn ON" : "Turn OFF");
			Serial.print(" (");
			Serial.print(lightSensor->getCurrentLux(), 1);
			Serial.print(" lux vs ");
			Serial.print(LIGHT_THRESHOLD_LUX, 1);
			Serial.println(" threshold)");
			
			/// We attempt the state change
			bool success = relayController->setRelayState(shouldTurnOn);
			
			if (success) {
				Serial.println("✓ Relay state changed successfully");
			} else {
				Serial.println("⏳ Relay change blocked by safety timer");
			}
		}
	}
	
	/// We display periodic status updates
	if (currentTime - lastStatusDisplay >= displayInterval) {
		lastStatusDisplay = currentTime;
		
		displaySystemStatus();
	}
	
	/// We add a small delay to prevent overwhelming the system
	delay(100);
}

/// We clean up memory on program end
void cleanup() {
	if (relayController) {
		relayController->emergencyStop();
		delete relayController;
		relayController = nullptr;
	}
	
	if (lightSensor) {
		delete lightSensor;
		lightSensor = nullptr;
	}
}