///
/// TimeManager Implementation
///
/// We rely on the ESP32 Arduino core's SNTP support: configTzTime() starts
/// lwIP's SNTP client in the background, which keeps the system clock in
/// UTC without any blocking calls from our side. Local time (including
/// DST) is derived on demand from a POSIX TZ string via localtime_r.
///

#include "timemanager.h"
#include "config.h"
#include <time.h>
#include <esp_sntp.h>
#include <string.h>

/// The system clock reads ~1970 (or boot-relative seconds) until the first
/// SNTP sync sets the real epoch - anything past late 2023 means we have
/// real time
static constexpr time_t MIN_PLAUSIBLE_EPOCH = 1700000000;

TimeManager::TimeManager(const char* ntpServer, const char* posixTz)
	: ntpServer(ntpServer)
	, lastSuccessfulSync(0)
	, syncCount(0)
	, started(false)
{
	/// We copy the TZ string since the caller's buffer (e.g. inside the
	/// config struct) may not outlive us
	strncpy(this->timezone, posixTz, sizeof(this->timezone) - 1);
	this->timezone[sizeof(this->timezone) - 1] = '\0';
}

void TimeManager::begin() {
	if (this->started) {
		return; /// Already started, e.g. WiFi reconnected without a reboot
	}
	this->started = true;

	/// configTzTime() applies the TZ string to libc, starts lwIP's SNTP
	/// client in the background and points it at our server - no socket
	/// code or blocking forceUpdate() on the loop task anymore
	configTzTime(this->timezone, this->ntpServer);

	/// The sync interval applies to the running SNTP instance, so it must
	/// come after configTzTime()
	sntp_set_sync_interval(NTP_UPDATE_INTERVAL_MS);

	Serial.println("TimeManager: SNTP client initialized");
	Serial.print("NTP server: ");
	Serial.println(this->ntpServer);
	Serial.print("Timezone: ");
	Serial.println(this->timezone);
}

void TimeManager::update() {
	if (!this->started) {
		return;
	}

	/// The status reads COMPLETED exactly once per sync and resets itself
	/// to RESET afterwards (IDF 4.4), so each completed sync is observed
	/// exactly once here - no callbacks needed
	if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
		this->lastSuccessfulSync = millis();
		this->syncCount++;

		Serial.println("TimeManager: ✓ Time sync successful");
		Serial.print("Current time: ");
		Serial.println(this->getCurrentTimeString());
		Serial.print("Current date: ");
		Serial.println(this->getCurrentDateString());
	}
}

bool TimeManager::hasValidTime() const {
	return this->started && time(nullptr) > MIN_PLAUSIBLE_EPOCH;
}

bool TimeManager::getLocalTime(struct tm& out) const {
	time_t now = time(nullptr);
	localtime_r(&now, &out);
	return hasValidTime();
}

int TimeManager::getCurrentHour() const {
	struct tm tm;
	if (!this->getLocalTime(tm)) {
		return -1; /// We return invalid value when time is not available
	}
	return tm.tm_hour;
}

int TimeManager::getCurrentMinute() const {
	struct tm tm;
	if (!this->getLocalTime(tm)) {
		return -1;
	}
	return tm.tm_min;
}

int TimeManager::getCurrentSecond() const {
	struct tm tm;
	if (!this->getLocalTime(tm)) {
		return -1;
	}
	return tm.tm_sec;
}

String TimeManager::getCurrentTimeString() const {
	struct tm tm;
	if (!this->getLocalTime(tm)) {
		return "No Time Available";
	}

	/// We format time as HH:MM:SS
	char timeBuffer[16];
	strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &tm);
	return String(timeBuffer);
}

String TimeManager::getCurrentDateString() const {
	struct tm tm;
	if (!this->getLocalTime(tm)) {
		return "No Date Available";
	}

	char dateBuffer[16];
	strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &tm);
	return String(dateBuffer);
}

unsigned long TimeManager::getTimeSinceLastSync() const {
	if (this->lastSuccessfulSync == 0) {
		return ULONG_MAX; /// We return max value if never synced
	}
	return millis() - this->lastSuccessfulSync;
}

unsigned long TimeManager::getSyncCount() const {
	return this->syncCount;
}

void TimeManager::setTimezone(const char* posixTz) {
	if (posixTz == nullptr || posixTz[0] == '\0') {
		return;
	}

	strncpy(this->timezone, posixTz, sizeof(this->timezone) - 1);
	this->timezone[sizeof(this->timezone) - 1] = '\0';

	/// Takes effect immediately - SNTP keeps syncing UTC, so no restart
	/// or resync is needed
	setenv("TZ", this->timezone, 1);
	tzset();

	Serial.print("TimeManager: Timezone set to ");
	Serial.println(this->timezone);
}

void TimeManager::attemptRecovery() {
	Serial.println("TimeManager: Attempting recovery - restarting SNTP...");

	if (!this->started) {
		return;
	}

	/// configTzTime() stops a running SNTP instance before starting a fresh
	/// one, so this doubles as a restart. Success only shows up via
	/// hasValidTime() once the new instance completes a sync
	configTzTime(this->timezone, this->ntpServer);
	sntp_set_sync_interval(NTP_UPDATE_INTERVAL_MS);
}

bool TimeManager::isStarted() const {
	return this->started;
}
