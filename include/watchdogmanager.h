///
/// WatchdogManager - Hardware task watchdog for hang detection
///
/// We use the ESP32's task watchdog to force a reset if the main loop ever
/// hangs (e.g. an I2C read that never returns), since that's a failure mode
/// no amount of software error handling inside a stuck loop can recover
/// from. SystemDiagnostics already tracks boot count and detects an
/// unexpected reboot after the fact via its own RTC marker; we complement
/// that by reading the hardware's actual reset reason (esp_reset_reason())
/// so a watchdog-triggered reset is distinguishable from a panic, brownout,
/// or manual reset instead of just "unexpected."
///

#ifndef WATCHDOGMANAGER_H
#define WATCHDOGMANAGER_H

#include <Arduino.h>
#include <esp_system.h>

class WatchdogManager {
public:
	explicit WatchdogManager(unsigned long timeoutMs);

	/// Subscribe the calling task to the hardware watchdog and record the
	/// reason for the previous reset
	/// We deliberately call this after the boot-time WiFi/time wait loops
	/// complete (see main.cpp) - those already use bounded, self-timing-out
	/// loops, and subscribing before them would false-trigger the watchdog
	/// on a slow but healthy WiFi connect
	void begin();

	/// Feed the watchdog - call once per loop() iteration
	/// We only reach this if the loop actually completed, proving it isn't hung
	void feed();

	/// Get a human-readable reason for the last system reset
	[[nodiscard]] const char* getLastResetReasonString() const;

	/// Check whether the last reset was caused by a genuine fault
	/// (watchdog/panic/brownout) rather than a normal power-on or manual reset
	[[nodiscard]] bool wasLastResetAFault() const;

private:
	unsigned long timeoutMs;
	esp_reset_reason_t lastResetReason;
};

#endif /// WATCHDOGMANAGER_H
