///
/// PlantController Implementation
///
/// We implement the core decision logic that combines time-based
/// scheduling with ambient light detection to provide intelligent
/// plant light control. The system prioritizes schedule compliance
/// while optimizing for energy efficiency. The decision rules themselves
/// live in controllogic.cpp (decide()) - this file gathers inputs from
/// the hardware components and executes the resulting decision.
///

#include "plantcontroller.h"
#include "config.h"

PlantController::PlantController(WiFiManager* wifiManager, TimeManager* timeManager,
							LightSensor* lightSensor, RelayController* relayController,
							SystemDiagnostics* diagnostics)
	: wifiManager(wifiManager)
	, timeManager(timeManager)
	, lightSensor(lightSensor)
	, relayController(relayController)
	, diagnostics(diagnostics)
	, lastDecision(ControlDecision::WaitForData)
	, lastReason(ControlReason::NoValidTime)
	, lastDecisionTime(0)
	, lastUpdateTime(0)
	, decisionCount(0)
	, relayChanges(0)
	, manualOverride(ManualOverride::Auto)
	, updateInterval(CHECK_INTERVAL_MS)
	, lastDecisionDeferred(false)
	, policy{LIGHT_START_HOUR, LIGHT_END_HOUR, LIGHT_THRESHOLD_LUX, 15.0}  /// Default 15 lux dead band
	, lastSensorRecoveryAttempt(0)
	, lastTimeRecoveryAttempt(0)
	, recoveryInterval(60000)  /// 1 minute cooldown between recovery attempts
	, wifiOutageRecorded(false)
{
	/// We initialize all member variables for clean state
}

void PlantController::begin() {
	Serial.println("PlantController: Initializing intelligent plant light control");

	/// We display the control configuration
	Serial.print("Schedule: ");
	Serial.print(this->policy.startHour);
	Serial.print(":00 to ");
	Serial.print(this->policy.endHour);
	Serial.println(":00");

	Serial.print("Light threshold: ");
	Serial.print(this->policy.thresholdLux);
	Serial.println(" lux");

	Serial.print("Update interval: ");
	Serial.print(this->updateInterval / 1000);
	Serial.println(" seconds");

	Serial.print("Manual override: ");
	Serial.println(toString(this->manualOverride));

	/// We perform initial evaluation
	this->forceUpdate();

	Serial.println("PlantController: ✓ Initialized and ready");
}

void PlantController::update() {
	/// We check if it's time for a control update
	unsigned long currentTime = millis();
	if (currentTime - this->lastUpdateTime < this->updateInterval) {
		return; /// Not time for update yet
	}

	this->lastUpdateTime = currentTime;

	/// We attempt component recovery if needed
	this->attemptComponentRecovery();

	/// We gather current inputs and decide what should happen
	ControlOutput out = decide(this->gatherInputs(), this->policy);

	/// We execute the decision if it's different from current state; a
	/// non-actionable decision also clears any earlier deferral, since the
	/// deferred switch is no longer wanted
	if (out.decision != ControlDecision::KeepCurrent && out.decision != ControlDecision::WaitForData) {
		this->executeDecision(out);
	} else {
		this->lastDecisionDeferred = false;
	}

	/// We update our state tracking
	this->lastDecision = out.decision;
	this->lastReason = out.reason;
	this->lastDecisionTime = currentTime;
	this->decisionCount++;
}

void PlantController::forceUpdate() {
	Serial.println("PlantController: Forcing immediate evaluation...");

	ControlOutput out = decide(this->gatherInputs(), this->policy);

	this->executeDecision(out);

	this->lastDecision = out.decision;
	this->lastReason = out.reason;
	this->lastDecisionTime = millis();
	this->decisionCount++;
}

ControlDecision PlantController::getLastDecision() const {
	return this->lastDecision;
}

ControlReason PlantController::getLastReason() const {
	return this->lastReason;
}

unsigned long PlantController::getLastDecisionTime() const {
	return this->lastDecisionTime;
}

bool PlantController::areAllComponentsHealthy() const {
	return this->timeManager->hasValidTime() && this->lightSensor->isSensorHealthy();
}

unsigned long PlantController::getDecisionCount() const {
	return this->decisionCount;
}

unsigned long PlantController::getRelayChanges() const {
	return this->relayChanges;
}

void PlantController::setManualOverride(ManualOverride mode) {
	this->manualOverride = mode;

	Serial.print("PlantController: Manual override set to ");
	Serial.println(toString(mode));

	/// We re-evaluate immediately so the new mode takes effect without
	/// waiting for the next scheduled update
	this->forceUpdate();
}

ManualOverride PlantController::getManualOverride() const {
	return this->manualOverride;
}

void PlantController::updateConfiguration(int startHour, int endHour, float thresholdLux, float hysteresisLux) {
	/// We update the configuration with new values
	this->policy.startHour = startHour;
	this->policy.endHour = endHour;
	this->policy.thresholdLux = thresholdLux;
	this->policy.hysteresisLux = hysteresisLux;

	Serial.println("PlantController: Configuration updated");
	Serial.print("  Schedule: ");
	Serial.print(this->policy.startHour);
	Serial.print(":00 to ");
	Serial.print(this->policy.endHour);
	Serial.println(":00");
	Serial.print("  Light threshold: ");
	Serial.print(this->policy.thresholdLux);
	Serial.println(" lux");
	Serial.print("  Hysteresis: ");
	Serial.print(this->policy.hysteresisLux);
	Serial.println(" lux");

	/// We force immediate re-evaluation with new settings
	this->forceUpdate();
}

bool PlantController::isLastDecisionDeferred() const {
	return this->lastDecisionDeferred;
}

ControlInputs PlantController::gatherInputs() const {
	return ControlInputs{
		this->manualOverride,
		this->timeManager->hasValidTime(),
		this->timeManager->getCurrentHour(),
		this->lightSensor->isSensorHealthy(),
		this->lightSensor->getCurrentLux(),
		this->relayController->getRelayState()
	};
}

void PlantController::executeDecision(const ControlOutput& out) {
	Serial.print("PlantController: Decision - ");
	Serial.print(toString(out.decision));
	Serial.print(" (");
	Serial.print(toString(out.reason));
	Serial.println(")");

	/// We handle each decision type
	switch (out.decision) {
		case ControlDecision::TurnOn:
			if (this->relayController->setRelayState(true)) {
				this->relayChanges++;
				this->lastDecisionDeferred = false;
				Serial.println("PlantController: ✓ Lights turned ON");
			} else {
				this->lastDecisionDeferred = true;
				Serial.println("PlantController: ⏳ Deferred - relay switch interval");
			}
			break;

		case ControlDecision::TurnOff:
			if (this->relayController->setRelayState(false)) {
				this->relayChanges++;
				this->lastDecisionDeferred = false;
				Serial.println("PlantController: ✓ Lights turned OFF");
			} else {
				this->lastDecisionDeferred = true;
				Serial.println("PlantController: ⏳ Deferred - relay switch interval");
			}
			break;

		case ControlDecision::KeepCurrent:
			this->lastDecisionDeferred = false;
			Serial.print("PlantController: ↔ Keeping current state (");
			Serial.print(this->relayController->getRelayState() ? "ON" : "OFF");
			Serial.println(")");
			break;

		case ControlDecision::WaitForData:
			this->lastDecisionDeferred = false;
			Serial.println("PlantController: ⏳ Waiting for valid data");
			break;
	}
}

void PlantController::attemptComponentRecovery() {
	unsigned long currentTime = millis();

	/// We check if light sensor needs recovery
	if (!this->lightSensor->isSensorHealthy()) {
		/// We only attempt recovery if cooldown period has passed
		if (currentTime - this->lastSensorRecoveryAttempt >= this->recoveryInterval) {
			this->lastSensorRecoveryAttempt = currentTime;

			Serial.println("PlantController: Sensor unhealthy, triggering recovery");
			this->diagnostics->recordFailure(ComponentType::LightSensor, "Sensor health check failed");

			bool recovered = this->lightSensor->attemptRecovery();
			this->diagnostics->recordRecovery(ComponentType::LightSensor, recovered);

			if (recovered) {
				Serial.println("PlantController: ✓ Sensor recovery successful");
			} else {
				Serial.println("PlantController: ✗ Sensor recovery failed");
			}
		}
	}

	/// We check if time manager needs recovery - only worth attempting while
	/// WiFi is up, since NTP without WiFi is a guaranteed failure that would
	/// just inflate the failure/recovery counters every cooldown period
	if (!this->timeManager->hasValidTime() && this->wifiManager->isConnected()) {
		/// We only attempt recovery if cooldown period has passed
		if (currentTime - this->lastTimeRecoveryAttempt >= this->recoveryInterval) {
			this->lastTimeRecoveryAttempt = currentTime;

			Serial.println("PlantController: Time invalid, triggering recovery");
			this->diagnostics->recordFailure(ComponentType::Time, "Time validation failed");

			bool recovered = this->timeManager->attemptRecovery();
			this->diagnostics->recordRecovery(ComponentType::Time, recovered);

			if (recovered) {
				Serial.println("PlantController: ✓ Time recovery successful");
			} else {
				Serial.println("PlantController: ✗ Time recovery failed");
			}
		}
	}

	/// WiFi manager already has auto-reconnection, we just track failures.
	/// We record an extended outage once per episode rather than every tick,
	/// since a day-long outage otherwise means thousands of redundant records
	if (!this->wifiManager->isConnected()) {
		unsigned long timeSinceLastConnection = this->wifiManager->getTimeSinceLastConnection();
		if (timeSinceLastConnection > 300000 && !this->wifiOutageRecorded) {  /// More than 5 minutes disconnected
			this->diagnostics->recordFailure(ComponentType::WiFi, "Extended disconnection");
			this->wifiOutageRecorded = true;
		}
	} else {
		this->wifiOutageRecorded = false;
	}
}
