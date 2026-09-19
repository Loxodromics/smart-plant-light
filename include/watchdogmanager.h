///
/// WatchdogManager - Hardware task watchdog for hang detection
///
/// We use the ESP32's task watchdog to force a reset if the main loop ever
/// hangs (e.g. an I2C read that never returns), since that's a failure mode
/// no amount of software error handling inside a stuck loop can recover
/// from. Reset-reason classification lives in SystemDiagnostics.
///

#ifndef WATCHDOGMANAGER_H
#define WATCHDOGMANAGER_H

#include <Arduino.h>

class WatchdogManager {
public:
	explicit WatchdogManager(unsigned long timeoutMs);

	/// Subscribe the calling task to the hardware watchdog
	/// We deliberately call this after the boot-time WiFi/time wait loops
	/// complete (see main.cpp) - those already use bounded, self-timing-out
	/// loops, and subscribing before them would false-trigger the watchdog
	/// on a slow but healthy WiFi connect
	void begin();

	/// Feed the watchdog - call once per loop() iteration
	/// We only reach this if the loop actually completed, proving it isn't hung
	void feed();

private:
	unsigned long timeoutMs;
};

#endif /// WATCHDOGMANAGER_H
