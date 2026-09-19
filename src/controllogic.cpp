///
/// ControlLogic Implementation
///
/// Pure decision logic - no Arduino/ESP32 dependencies, so this file
/// compiles and runs in the host-side test env (pio test -e native).
///

#include "controllogic.h"

bool isTimeInSchedule(int minutesSinceMidnight, int startMinutes, int endMinutes) {
	/// Equal times mean the schedule never turns itself off (24-hour operation)
	if (startMinutes == endMinutes) {
		return true;
	}

	/// Normal ranges (e.g., 08:00 to 22:00)
	if (startMinutes < endMinutes) {
		return minutesSinceMidnight >= startMinutes && minutesSinceMidnight < endMinutes;
	}

	/// Ranges that cross midnight (e.g., 22:00 to 06:00): lights are in-schedule
	/// from 22:00 through 06:00 overnight
	return minutesSinceMidnight >= startMinutes || minutesSinceMidnight < endMinutes;
}

bool isAmbientLightLow(float lux, bool relayOn, float thresholdLux, float hysteresisLux) {
	float upperThreshold = thresholdLux + hysteresisLux;
	float lowerThreshold = thresholdLux - hysteresisLux;

	if (relayOn) {
		/// Relay is ON - keep lights on unless we exceed the upper threshold
		return lux < upperThreshold;
	}
	/// Relay is OFF - turn lights on only once we drop below the lower threshold
	return lux < lowerThreshold;
}

ControlOutput decide(const ControlInputs& in, const ControlPolicy& policy) {
	/// Manual override bypasses schedule/light logic entirely so it keeps
	/// working even if time sync or the sensor is down
	if (in.override != ManualOverride::Auto) {
		bool wantOn = (in.override == ManualOverride::ForceOn);
		ControlReason reason = wantOn ? ControlReason::ManualOverrideOn : ControlReason::ManualOverrideOff;

		if (in.relayOn == wantOn) {
			return {ControlDecision::KeepCurrent, reason};
		}
		return {wantOn ? ControlDecision::TurnOn : ControlDecision::TurnOff, reason};
	}

	if (!in.timeValid) {
		return {ControlDecision::WaitForData, ControlReason::NoValidTime};
	}

	if (!isTimeInSchedule(in.minutesSinceMidnight, policy.startMinutes, policy.endMinutes)) {
		/// Outside the schedule the sensor is irrelevant - lights-off is the
		/// safe state even if the sensor is unhealthy
		return {in.relayOn ? ControlDecision::TurnOff : ControlDecision::KeepCurrent, ControlReason::OutOfSchedule};
	}

	if (!in.sensorHealthy) {
		return {ControlDecision::WaitForData, ControlReason::SensorFailure};
	}

	if (isAmbientLightLow(in.lux, in.relayOn, policy.thresholdLux, policy.hysteresisLux)) {
		return {in.relayOn ? ControlDecision::KeepCurrent : ControlDecision::TurnOn, ControlReason::InScheduleDark};
	}
	return {in.relayOn ? ControlDecision::TurnOff : ControlDecision::KeepCurrent, ControlReason::InScheduleBright};
}

const char* toString(ControlDecision decision) {
	switch (decision) {
		case ControlDecision::TurnOn: return "TURN ON";
		case ControlDecision::TurnOff: return "TURN OFF";
		case ControlDecision::KeepCurrent: return "KEEP CURRENT";
		case ControlDecision::WaitForData: return "WAIT FOR DATA";
		default: return "UNKNOWN";
	}
}

const char* toString(ControlReason reason) {
	switch (reason) {
		case ControlReason::OutOfSchedule: return "Outside schedule";
		case ControlReason::InScheduleDark: return "In schedule + dark";
		case ControlReason::InScheduleBright: return "In schedule + bright";
		case ControlReason::NoValidTime: return "No valid time";
		case ControlReason::SensorFailure: return "Sensor failure";
		case ControlReason::ManualOverrideOn: return "Manual override: ON";
		case ControlReason::ManualOverrideOff: return "Manual override: OFF";
		default: return "Unknown reason";
	}
}

const char* toString(ManualOverride mode) {
	switch (mode) {
		case ManualOverride::Auto: return "AUTO";
		case ManualOverride::ForceOn: return "FORCE ON";
		case ManualOverride::ForceOff: return "FORCE OFF";
		default: return "UNKNOWN";
	}
}
