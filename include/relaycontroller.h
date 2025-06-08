///
/// RelayController - Manages relay switching with safety features
/// 
/// We implement debouncing and minimum switch intervals to prevent
/// rapid relay cycling which could damage the relay contacts or
/// connected equipment. The class tracks state changes and enforces
/// safety delays between operations.
///

#ifndef RELAYCONTROLLER_H
#define RELAYCONTROLLER_H

#include <Arduino.h>

class RelayController {
public:
	explicit RelayController(int relayPin);
	
	/// Initialize the relay controller and set initial state
	/// We set the relay to OFF state during initialization for safety
	void begin();
	
	/// Request relay state change with safety checks
	/// Returns true if state change was allowed, false if blocked by safety logic
	[[nodiscard]] bool setRelayState(bool state);
	
	/// Get current relay state without triggering any changes
	[[nodiscard]] bool getRelayState() const;
	
	/// Check if enough time has passed since last state change
	/// We use this to prevent rapid switching that could damage equipment
	[[nodiscard]] bool canSwitchRelay() const;
	
	/// Get time since last state change in milliseconds
	[[nodiscard]] unsigned long getTimeSinceLastSwitch() const;
	
	/// Force relay to OFF state immediately (emergency stop)
	/// We bypass safety delays in emergency situations
	void emergencyStop();

private:
	const int relayPin;
	bool currentState;
	unsigned long lastSwitchTime;
	unsigned long minSwitchInterval;
	
	/// Actually change the relay hardware state
	/// We separate this from the public interface to control when it happens
	void updateRelayHardware(bool state);
};

#endif /// RELAYCONTROLLER_H