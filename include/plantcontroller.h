///
/// PlantController - Main decision logic for smart plant light control
///
/// We integrate all components (WiFi, time, light sensor, relay) to make
/// intelligent decisions about when to turn plant lights on or off.
/// The controller implements a two-stage decision process: time-based
/// scheduling combined with ambient light level detection. The actual
/// decision rules live in controllogic.h/decide() - this class is the
/// Arduino-side adapter that gathers inputs from the hardware components
/// and executes the resulting decision on the relay.
///

#ifndef PLANTCONTROLLER_H
#define PLANTCONTROLLER_H

#include <Arduino.h>
#include "wifimanager.h"
#include "timemanager.h"
#include "lightsensor.h"
#include "relaycontroller.h"
#include "systemdiagnostics.h"
#include "controllogic.h"

class PlantController {
public:
	PlantController(WiFiManager* wifiManager, TimeManager* timeManager,
				LightSensor* lightSensor, RelayController* relayController,
				SystemDiagnostics* diagnostics);

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

	/// Set manual override mode (Auto / ForceOn / ForceOff)
	/// We use this for the software on/off switch - it bypasses the
	/// schedule and light-level logic entirely, but still respects the
	/// relay's minimum switch interval. Not persisted - always resets to
	/// Auto on reboot, matching the rest of the system's safety-first
	/// "known-good state after any reset" pattern
	void setManualOverride(ManualOverride mode);

	/// Get the current manual override mode
	[[nodiscard]] ManualOverride getManualOverride() const;

	/// Attempt to recover from component failures
	/// We check for failures and trigger recovery attempts
	void attemptComponentRecovery();

	/// Update configuration from ConfigManager
	/// We allow runtime configuration changes without recompiling
	void updateConfiguration(int startHour, int endHour, float thresholdLux, float hysteresisLux);

	/// Check whether the last TurnOn/TurnOff decision was deferred because
	/// the relay's minimum switch interval hadn't elapsed yet. update() runs
	/// every CHECK_INTERVAL_MS, so a deferred decision is retried naturally
	[[nodiscard]] bool isLastDecisionDeferred() const;

private:
	/// Component references
	WiFiManager* wifiManager;
	TimeManager* timeManager;
	LightSensor* lightSensor;
	RelayController* relayController;
	SystemDiagnostics* diagnostics;

	/// Control state
	ControlDecision lastDecision;
	ControlReason lastReason;
	unsigned long lastDecisionTime;
	unsigned long lastUpdateTime;
	unsigned long decisionCount;
	unsigned long relayChanges;
	ManualOverride manualOverride;
	unsigned long updateInterval;
	bool lastDecisionDeferred;

	/// Schedule and light-level policy passed to decide()
	ControlPolicy policy;

	/// Recovery tracking
	unsigned long lastSensorRecoveryAttempt;
	unsigned long lastTimeRecoveryAttempt;
	unsigned long recoveryInterval;  /// Cooldown period between recovery attempts

	/// Read the current state of all components into a ControlInputs snapshot
	[[nodiscard]] ControlInputs gatherInputs() const;

	/// Execute the control decision
	/// We handle the actual relay switching with proper logging
	void executeDecision(const ControlOutput& out);
};

#endif /// PLANTCONTROLLER_H
