///
/// PlantController - Main decision logic for smart plant light control
/// 
/// We integrate all components (WiFi, time, light sensor, relay) to make
/// intelligent decisions about when to turn plant lights on or off.
/// The controller implements a two-stage decision process: time-based
/// scheduling combined with ambient light level detection.
///

#ifndef PLANTCONTROLLER_H
#define PLANTCONTROLLER_H

#include <Arduino.h>
#include "wifimanager.h"
#include "timemanager.h"
#include "lightsensor.h"
#include "relaycontroller.h"

enum class ControlDecision {
	TurnOn,          /// Lights should be ON (in schedule + dark)
	TurnOff,         /// Lights should be OFF (out of schedule OR bright)
	KeepCurrent,     /// No change needed (current state is correct)
	WaitForData      /// Cannot decide (missing sensor data or time)
};

enum class ControlReason {
	OutOfSchedule,       /// Outside time window
	InScheduleDark,      /// In schedule and ambient light is low
	InScheduleBright,    /// In schedule but ambient light is sufficient
	NoValidTime,         /// Time synchronization not available
	SensorFailure,       /// Light sensor not working
	RelayBusy           /// Relay cannot switch (safety interval)
};

class PlantController {
public:
	PlantController(WiFiManager* wifiManager, TimeManager* timeManager, 
				LightSensor* lightSensor, RelayController* relayController);
	
	/// Initialize the plant controller
	/// We set up initial state and validate all components
	void begin();
	
	/// Main control loop - analyze conditions and make decisions
	/// We check all inputs and decide whether to change relay state
	void update();
	
	/// Force immediate evaluation and relay update if needed
	/// We use this for manual override or immediate response
	void forceUpdate();
	
	/// Get the last control decision made
	[[nodiscard]] ControlDecision getLastDecision() const;
	
	/// Get the reason for the last decision
	[[nodiscard]] ControlReason getLastReason() const;
	
	/// Get timestamp of last decision in milliseconds
	[[nodiscard]] unsigned long getLastDecisionTime() const;
	
	/// Check if all required components are healthy
	[[nodiscard]] bool areAllComponentsHealthy() const;
	
	/// Get number of successful control decisions made
	[[nodiscard]] unsigned long getDecisionCount() const;
	
	/// Get number of actual relay state changes made
	[[nodiscard]] unsigned long getRelayChanges() const;
	
	/// Enable or disable automatic control
	/// We allow manual override when needed
	void setAutomaticControl(bool enabled);
	
	/// Check if automatic control is currently enabled
	[[nodiscard]] bool isAutomaticControlEnabled() const;

private:
	/// Component references
	WiFiManager* wifiManager;
	TimeManager* timeManager;
	LightSensor* lightSensor;
	RelayController* relayController;
	
	/// Control state
	ControlDecision lastDecision;
	ControlReason lastReason;
	unsigned long lastDecisionTime;
	unsigned long lastUpdateTime;
	unsigned long decisionCount;
	unsigned long relayChanges;
	bool automaticControlEnabled;
	unsigned long updateInterval;
	
	/// Configuration
	int scheduleStartHour;
	int scheduleEndHour;
	float lightThresholdLux;
	
	/// Core decision logic methods
	/// We break down the decision process into clear steps
	[[nodiscard]] ControlDecision analyzeConditions(ControlReason& reason) const;
	[[nodiscard]] bool isWithinSchedule() const;
	[[nodiscard]] bool isAmbientLightLow() const;
	[[nodiscard]] bool shouldRelayBeOn() const;
	
	/// Execute the control decision
	/// We handle the actual relay switching with proper logging
	void executeDecision(ControlDecision decision, ControlReason reason);
	
	/// Validate component health
	[[nodiscard]] bool validateComponents(ControlReason& reason) const;
	
	/// Get descriptive string for decision type
	[[nodiscard]] const char* getDecisionString(ControlDecision decision) const;
	
	/// Get descriptive string for decision reason
	[[nodiscard]] const char* getReasonString(ControlReason reason) const;
};

#endif /// PLANTCONTROLLER_H