///
/// WatchdogManager Implementation
///

#include "watchdogmanager.h"
#include <esp_task_wdt.h>

WatchdogManager::WatchdogManager(unsigned long timeoutMs)
	: timeoutMs(timeoutMs)
	, lastResetReason(ESP_RST_UNKNOWN)
{
}

void WatchdogManager::begin() {
	this->lastResetReason = esp_reset_reason();

	Serial.print("WatchdogManager: Last reset reason - ");
	Serial.println(this->getLastResetReasonString());

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

const char* WatchdogManager::getLastResetReasonString() const {
	switch (this->lastResetReason) {
		case ESP_RST_POWERON:   return "Power-on reset";
		case ESP_RST_EXT:       return "External pin reset";
		case ESP_RST_SW:        return "Software reset";
		case ESP_RST_PANIC:     return "Software panic (crash)";
		case ESP_RST_INT_WDT:   return "Interrupt watchdog";
		case ESP_RST_TASK_WDT:  return "Task watchdog (hang detected)";
		case ESP_RST_WDT:       return "Other watchdog";
		case ESP_RST_DEEPSLEEP: return "Woke from deep sleep";
		case ESP_RST_BROWNOUT:  return "Brownout (power dip)";
		case ESP_RST_SDIO:      return "SDIO reset";
		default:                return "Unknown reset reason";
	}
}

bool WatchdogManager::wasLastResetAFault() const {
	switch (this->lastResetReason) {
		case ESP_RST_PANIC:
		case ESP_RST_INT_WDT:
		case ESP_RST_TASK_WDT:
		case ESP_RST_WDT:
		case ESP_RST_BROWNOUT:
			return true;
		default:
			return false;
	}
}
