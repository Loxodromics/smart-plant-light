///
/// Main Test Program for Relay Controller
/// 
/// We test the relay controller by cycling it on/off with safety delays.
/// This allows us to verify both the hardware control and safety features
/// before integrating with other components.
///

#include <Arduino.h>
#include "relaycontroller.h"
#include "config.h"

RelayController* relayController;

void setup() {
	/// We initialize serial communication for debugging output
	Serial.begin(115200);
	while (!Serial) {
		/// We wait for serial port to be ready (important for some boards)
		delay(10);
	}
	
	Serial.println("\n=== Smart Plant Light Controller - Relay Test ===");
	Serial.println("Testing relay controller with safety features...");
	
	/// We create and initialize the relay controller
	relayController = new RelayController(RELAY_PIN);
	relayController->begin();
	
	Serial.print("Relay pin configured: GPIO");
	Serial.println(RELAY_PIN);
	Serial.print("Minimum switch interval: ");
	Serial.print(MIN_SWITCH_INTERVAL_MS);
	Serial.println("ms");
	Serial.println("Starting relay test cycle in 3 seconds...");
	
	delay(3000);
}

void loop() {
	static unsigned long lastTestTime = 0;
	static bool testState = false;
	const unsigned long testInterval = 5000; /// We test every 5 seconds
	
	unsigned long currentTime = millis();
	
	/// We run our test cycle every 5 seconds
	if (currentTime - lastTestTime >= testInterval) {
		lastTestTime = currentTime;
		
		/// We toggle the test state for the next attempt
		testState = !testState;
		
		Serial.print("\n--- Test Cycle ---");
		Serial.print(" Attempting to turn relay ");
		Serial.println(testState ? "ON" : "OFF");
		
		/// We attempt to change relay state and report the result
		bool success = relayController->setRelayState(testState);
		
		if (success) {
			Serial.println("✓ State change successful");
		} else {
			Serial.println("✗ State change blocked by safety logic");
			/// We revert testState since the change didn't happen
			testState = !testState;
		}
		
		/// We report current status
		Serial.print("Current relay state: ");
		Serial.println(relayController->getRelayState() ? "ON" : "OFF");
		Serial.print("Time since last switch: ");
		Serial.print(relayController->getTimeSinceLastSwitch());
		Serial.println("ms");
		Serial.print("Can switch relay: ");
		Serial.println(relayController->canSwitchRelay() ? "YES" : "NO");
	}
	
	/// We add a small delay to prevent overwhelming the serial output
	delay(100);
}

/// We clean up memory on program end (though this rarely happens on ESP32)
void cleanup() {
	if (relayController) {
		relayController->emergencyStop();
		delete relayController;
		relayController = nullptr;
	}
}