///
/// WatchdogManager Implementation
///

#include "watchdogmanager.h"
#include <esp_task_wdt.h>

WatchdogManager::WatchdogManager(unsigned long timeoutMs)
	: timeoutMs(timeoutMs)
{
}

void WatchdogManager::begin() {
	/// We (re)configure the task watchdog with our timeout and subscribe the
	/// current task (the Arduino loop task) so a hang here triggers a reset.
	/// esp_task_wdt_init() updates the existing configuration rather than
	/// erroring if the framework already initialized the TWDT for its idle
	/// tasks, so this is safe regardless of the framework's default state
	esp_task_wdt_init(this->timeoutMs / 1000, true);
	esp_task_wdt_add(NULL);

	Serial.print("WatchdogManager: ✓ Initialized (timeout: ");
	Serial.print(this->timeoutMs / 1000);
	Serial.println("s)");
}

void WatchdogManager::feed() {
	esp_task_wdt_reset();
}
