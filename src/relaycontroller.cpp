///
/// RelayController Implementation
/// 
/// We implement safety features to protect both the relay hardware
/// and connected equipment from damage due to rapid switching.
/// The controller enforces minimum time intervals between state changes.
///

#include "relaycontroller.h"
#include "config.h"

RelayController::RelayController(int relayPin) 
	: relayPin(relayPin)
	, currentState(false)
	, lastSwitchTime(0)
	, minSwitchInterval(MIN_SWITCH_INTERVAL_MS)
{
	/// We initialize all member variables in the constructor initializer list
	/// for better performance and to ensure consistent initialization order
}

void RelayController::begin() {
	/// We configure the pin as output and set initial safe state
	pinMode(this->relayPin, OUTPUT);
	
	/// We start with relay OFF for safety - this ensures we don't accidentally
	/// turn on equipment during startup before all systems are ready
	this->updateRelayHardware(false);
	this->currentState = false;
	this->lastSwitchTime = millis();
	
	Serial.println("RelayController: Initialized with relay OFF");
}

bool RelayController::setRelayState(bool state) {
	/// We check if the requested state is different from current state
	if (state == this->currentState) {
		/// No change needed, we return true to indicate success
		return true;
	}
	
	/// We enforce minimum time interval between switches to protect hardware
	if (!this->canSwitchRelay()) {
		Serial.print("RelayController: Switch blocked - minimum interval not met. Time since last: ");
		Serial.print(this->getTimeSinceLastSwitch());
		Serial.println("ms");
		return false;
	}
	
	/// We proceed with the state change since safety checks passed
	this->updateRelayHardware(state);
	this->currentState = state;
	this->lastSwitchTime = millis();
	
	Serial.print("RelayController: State changed to ");
	Serial.println(state ? "ON" : "OFF");
	
	return true;
}

bool RelayController::getRelayState() const {
	return this->currentState;
}

bool RelayController::canSwitchRelay() const {
	/// We check if enough time has elapsed since the last switch
	/// This prevents rapid cycling that could damage relay contacts
	return this->getTimeSinceLastSwitch() >= this->minSwitchInterval;
}

unsigned long RelayController::getTimeSinceLastSwitch() const {
	/// We handle potential millis() rollover by using unsigned arithmetic
	/// This works correctly even when millis() wraps around after ~49 days
	return millis() - this->lastSwitchTime;
}

void RelayController::emergencyStop() {
	/// We bypass all safety delays in emergency situations
	/// This is for situations where immediate shutdown is critical
	this->updateRelayHardware(false);
	this->currentState = false;
	this->lastSwitchTime = millis();
	
	Serial.println("RelayController: EMERGENCY STOP activated");
}

void RelayController::updateRelayHardware(bool state) {
	/// We write directly to the GPIO pin to control the relay
	/// LOW = relay OFF (normally open contacts open)
	/// HIGH = relay ON (normally open contacts closed)
	digitalWrite(this->relayPin, state ? HIGH : LOW);
}