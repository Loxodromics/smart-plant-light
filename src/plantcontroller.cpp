///
/// PlantController Implementation
/// 
/// We implement the core decision logic that combines time-based
/// scheduling with ambient light detection to provide intelligent
/// plant light control. The system prioritizes schedule compliance
/// while optimizing for energy efficiency.
///

#include "plantcontroller.h"
#include "config.h"

PlantController::PlantController(WiFiManager* wifiManager, TimeManager* timeManager, 
							LightSensor* lightSensor, RelayController* relayController)
	: wifiManager(wifiManager)
	, timeManager(timeManager)
	, lightSensor(lightSensor)
	, relayController(relayController)
	, lastDecision(ControlDecision::WaitForData)
	, lastReason(ControlReason::NoValidTime)
	, lastDecisionTime(0)
	, lastUpdateTime(0)
	, decisionCount(0)
	, relayChanges(0)
	, automaticControlEnabled(true)
	, updateInterval(CHECK_INTERVAL_MS)
	, scheduleStartHour(LIGHT_START_HOUR)
	, scheduleEndHour(LIGHT_END_HOUR)
	, lightThresholdLux(LIGHT_THRESHOLD_LUX)
{
	/// We initialize all member variables for clean state
}

void PlantController::begin() {
	Serial.println("PlantController: Initializing intelligent plant light control");
	
	/// We display the control configuration
	Serial.print("Schedule: ");
	Serial.print(this->scheduleStartHour);
	Serial.print(":00 to ");
	Serial.print(this->scheduleEndHour);
	Serial.println(":00");
	
	Serial.print("Light threshold: ");
	Serial.print(this->lightThresholdLux);
	Serial.println(" lux");
	
	Serial.print("Update interval: ");
	Serial.print(this->updateInterval / 1000);
	Serial.println(" seconds");
	
	Serial.print("Automatic control: ");
	Serial.println(this->automaticControlEnabled ? "ENABLED" : "DISABLED");
	
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
	
	/// We skip updates if automatic control is disabled
	if (!this->automaticControlEnabled) {
		return;
	}
	
	/// We analyze current conditions and make decision
	ControlReason reason;
	ControlDecision decision = this->analyzeConditions(reason);
	
	/// We execute the decision if it's different from current state
	if (decision != ControlDecision::KeepCurrent && decision != ControlDecision::WaitForData) {
		this->executeDecision(decision, reason);
	}
	
	/// We update our state tracking
	this->lastDecision = decision;
	this->lastReason = reason;
	this->lastDecisionTime = currentTime;
	this->decisionCount++;
}

void PlantController::forceUpdate() {
	Serial.println("PlantController: Forcing immediate evaluation...");
	
	ControlReason reason;
	ControlDecision decision = this->analyzeConditions(reason);
	
	this->executeDecision(decision, reason);
	
	this->lastDecision = decision;
	this->lastReason = reason;
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
	ControlReason dummyReason;
	return this->validateComponents(dummyReason);
}

unsigned long PlantController::getDecisionCount() const {
	return this->decisionCount;
}

unsigned long PlantController::getRelayChanges() const {
	return this->relayChanges;
}

void PlantController::setAutomaticControl(bool enabled) {
	this->automaticControlEnabled = enabled;
	Serial.print("PlantController: Automatic control ");
	Serial.println(enabled ? "ENABLED" : "DISABLED");
	
	if (!enabled) {
		/// We turn off lights when disabling automatic control for safety
		Serial.println("PlantController: Turning off lights (automatic control disabled)");
		this->relayController->setRelayState(false);
	}
}

bool PlantController::isAutomaticControlEnabled() const {
	return this->automaticControlEnabled;
}

ControlDecision PlantController::analyzeConditions(ControlReason& reason) const {
	/// We first validate that all components are working
	if (!this->validateComponents(reason)) {
		return ControlDecision::WaitForData;
	}
	
	/// We check if we're within the scheduled time window
	if (!this->isWithinSchedule()) {
		reason = ControlReason::OutOfSchedule;
		return this->relayController->getRelayState() ? ControlDecision::TurnOff : ControlDecision::KeepCurrent;
	}
	
	/// We're in schedule, so check ambient light conditions
	bool ambientLightLow = this->isAmbientLightLow();
	bool relayCurrentlyOn = this->relayController->getRelayState();
	
	if (ambientLightLow) {
		/// We want lights on because it's dark
		reason = ControlReason::InScheduleDark;
		return relayCurrentlyOn ? ControlDecision::KeepCurrent : ControlDecision::TurnOn;
	} else {
		/// We want lights off because there's sufficient ambient light
		reason = ControlReason::InScheduleBright;
		return relayCurrentlyOn ? ControlDecision::TurnOff : ControlDecision::KeepCurrent;
	}
}

bool PlantController::isWithinSchedule() const {
	/// We delegate to the time manager for schedule checking
	return this->timeManager->isTimeInRange(this->scheduleStartHour, this->scheduleEndHour);
}

bool PlantController::isAmbientLightLow() const {
	/// We use the light sensor's threshold comparison
	return this->lightSensor->isBelowThreshold(this->lightThresholdLux);
}

bool PlantController::shouldRelayBeOn() const {
	/// We combine schedule and light conditions
	return this->isWithinSchedule() && this->isAmbientLightLow();
}

void PlantController::executeDecision(ControlDecision decision, ControlReason reason) {
	Serial.print("PlantController: Decision - ");
	Serial.print(this->getDecisionString(decision));
	Serial.print(" (");
	Serial.print(this->getReasonString(reason));
	Serial.println(")");
	
	/// We handle each decision type
	switch (decision) {
		case ControlDecision::TurnOn:
			if (this->relayController->setRelayState(true)) {
				this->relayChanges++;
				Serial.println("PlantController: ✓ Lights turned ON");
			} else {
				Serial.println("PlantController: ⏳ Cannot turn ON (relay safety interval)");
			}
			break;
			
		case ControlDecision::TurnOff:
			if (this->relayController->setRelayState(false)) {
				this->relayChanges++;
				Serial.println("PlantController: ✓ Lights turned OFF");
			} else {
				Serial.println("PlantController: ⏳ Cannot turn OFF (relay safety interval)");
			}
			break;
			
		case ControlDecision::KeepCurrent:
			Serial.print("PlantController: ↔ Keeping current state (");
			Serial.print(this->relayController->getRelayState() ? "ON" : "OFF");
			Serial.println(")");
			break;
			
		case ControlDecision::WaitForData:
			Serial.println("PlantController: ⏳ Waiting for valid data");
			break;
	}
}

bool PlantController::validateComponents(ControlReason& reason) const {
	/// We check time manager health
	if (!this->timeManager->hasValidTime()) {
		reason = ControlReason::NoValidTime;
		return false;
	}
	
	/// We check light sensor health
	if (!this->lightSensor->isSensorHealthy()) {
		reason = ControlReason::SensorFailure;
		return false;
	}
	
	/// We check if relay can be controlled
	if (!this->relayController->canSwitchRelay()) {
		reason = ControlReason::RelayBusy;
		return false;
	}
	
	/// All components are healthy
	return true;
}

const char* PlantController::getDecisionString(ControlDecision decision) const {
	switch (decision) {
		case ControlDecision::TurnOn: return "TURN ON";
		case ControlDecision::TurnOff: return "TURN OFF";
		case ControlDecision::KeepCurrent: return "KEEP CURRENT";
		case ControlDecision::WaitForData: return "WAIT FOR DATA";
		default: return "UNKNOWN";
	}
}

const char* PlantController::getReasonString(ControlReason reason) const {
	switch (reason) {
		case ControlReason::OutOfSchedule: return "Outside schedule";
		case ControlReason::InScheduleDark: return "In schedule + dark";
		case ControlReason::InScheduleBright: return "In schedule + bright";
		case ControlReason::NoValidTime: return "No valid time";
		case ControlReason::SensorFailure: return "Sensor failure";
		case ControlReason::RelayBusy: return "Relay busy";
		default: return "Unknown reason";
	}
}