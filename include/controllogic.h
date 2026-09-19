///
/// ControlLogic - Pure decision logic for the plant light controller
///
/// This header has no Arduino/ESP32 dependencies so it can be compiled
/// and unit tested on the host (see test/test_controllogic). PlantController
/// is the Arduino-side adapter: it gathers a ControlInputs snapshot from the
/// hardware components and calls decide().
///

#ifndef CONTROLLOGIC_H
#define CONTROLLOGIC_H

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
	ManualOverrideOn,    /// Manually forced on
	ManualOverrideOff    /// Manually forced off
};

enum class ManualOverride {
	Auto,      /// Automatic schedule + light-level control (default)
	ForceOn,   /// Manually forced ON regardless of schedule/light level
	ForceOff   /// Manually forced OFF regardless of schedule/light level
};

/// Snapshot of everything decide() needs, gathered by the caller each tick
struct ControlInputs {
	ManualOverride override;
	bool timeValid;
	int hour;
	bool sensorHealthy;
	float lux;
	bool relayOn;
};

/// Schedule and light-level policy, set from ConfigManager/config.h
struct ControlPolicy {
	int startHour;
	int endHour;
	float thresholdLux;
	float hysteresisLux;
};

struct ControlOutput {
	ControlDecision decision;
	ControlReason reason;
};

/// Check whether an hour falls within [startHour, endHour)
/// start == end means 24-hour operation (always true); start > end means
/// an overnight window that wraps past midnight
[[nodiscard]] bool isHourInSchedule(int hour, int startHour, int endHour);

/// Check whether ambient light is low enough to want the lights on
/// Hysteresis is applied symmetrically around thresholdLux: relay on keeps
/// lights on below threshold + hysteresis, relay off turns lights on only
/// below threshold - hysteresis
[[nodiscard]] bool isAmbientLightLow(float lux, bool relayOn, float thresholdLux, float hysteresisLux);

/// Decide what the relay should do given the current inputs and policy
/// The anti-chatter minimum switch interval is an execution concern handled
/// by the caller (RelayController) - decide() only answers "what do we want"
[[nodiscard]] ControlOutput decide(const ControlInputs& in, const ControlPolicy& policy);

[[nodiscard]] const char* toString(ControlDecision decision);
[[nodiscard]] const char* toString(ControlReason reason);
[[nodiscard]] const char* toString(ManualOverride mode);

#endif /// CONTROLLOGIC_H
