///
/// TimeManager Implementation
/// 
/// We implement reliable NTP time synchronization for the plant
/// light controller. This ensures accurate time-based scheduling
/// even if the device loses power or WiFi connection temporarily.
///

#include "timemanager.h"
#include "config.h"
#include <time.h>

TimeManager::TimeManager(const char* ntpServer, int timezoneOffsetHours)
	: ntpServer(ntpServer)
	, timezoneOffsetSeconds(timezoneOffsetHours * 3600)
	, syncInterval(NTP_UPDATE_INTERVAL_MS)
	, lastSyncAttempt(0)
	, lastSuccessfulSync(0)
	, syncCount(0)
	, failedSyncCount(0)
	, timeValid(false)
	, started(false)
{
	/// We create the NTP client with our timezone offset
	this->ntpClient = new NTPClient(this->ntpUDP, this->ntpServer, this->timezoneOffsetSeconds);
}

TimeManager::~TimeManager() {
	/// We clean up dynamically allocated NTP client
	delete this->ntpClient;
}

void TimeManager::begin() {
	if (this->started) {
		return; /// Already started, e.g. WiFi reconnected without a reboot
	}
	this->started = true;

	/// We initialize the NTP client. We drive sync timing ourselves
	/// (update()/shouldAttemptSync()), so the library's own update interval
	/// is irrelevant - we never call its auto-retrying update()
	this->ntpClient->begin();

	Serial.println("TimeManager: NTP client initialized");
	Serial.print("NTP server: ");
	Serial.println(this->ntpServer);
	Serial.print("Timezone offset: ");
	Serial.print(this->timezoneOffsetSeconds / 3600);
	Serial.println(" hours");
	
	/// We attempt initial time sync
	bool syncResult = this->syncTime();
	if (!syncResult) {
		Serial.println("TimeManager: Initial sync failed, will retry later");
	}
}

void TimeManager::update() {
	if (!this->started) {
		return;
	}

	/// We check if it's time for a sync. This is the only sync trigger -
	/// we deliberately never call the library's own update(), which
	/// force-syncs (blocking up to 1s) on every call while unsynced
	if (this->needsSync() && this->shouldAttemptSync()) {
		Serial.println("TimeManager: Performing scheduled sync...");
		bool syncResult = this->syncTime();
		if (!syncResult) {
			Serial.println("TimeManager: Scheduled sync failed, will retry later");
		}
	}
}

bool TimeManager::syncTime() {
	if (!this->started) {
		return false; /// NTP client isn't begun until WiFi first connects
	}

	this->lastSyncAttempt = millis();
	
	Serial.println("TimeManager: Synchronizing with NTP server...");
	
	/// We force an update from the NTP client
	bool success = this->ntpClient->forceUpdate();
	
	if (success) {
		this->lastSuccessfulSync = millis();
		this->syncCount++;
		this->timeValid = true;

		Serial.println("TimeManager: ✓ Time sync successful");
		Serial.print("Current time: ");
		Serial.println(this->getCurrentTimeString());
		Serial.print("Current date: ");
		Serial.println(this->getCurrentDateString());

		return true;
	} else {
		this->failedSyncCount++;
		Serial.println("TimeManager: ✗ Time sync failed");
		/// We don't invalidate existing time on failure - keep using last known time
		return false;
	}
}

bool TimeManager::hasValidTime() const {
	return this->timeValid && this->ntpClient->isTimeSet();
}

int TimeManager::getCurrentHour() const {
	if (!this->hasValidTime()) {
		return -1; /// We return invalid value when time is not available
	}
	return this->ntpClient->getHours();
}

int TimeManager::getCurrentMinute() const {
	if (!this->hasValidTime()) {
		return -1;
	}
	return this->ntpClient->getMinutes();
}

int TimeManager::getCurrentSecond() const {
	if (!this->hasValidTime()) {
		return -1;
	}
	return this->ntpClient->getSeconds();
}

String TimeManager::getCurrentTimeString() const {
	if (!this->hasValidTime()) {
		return "No Time Available";
	}
	
	/// We format time as HH:MM:SS
	char timeBuffer[16];
	snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d",
			this->ntpClient->getHours(),
			this->ntpClient->getMinutes(),
			this->ntpClient->getSeconds());
	return String(timeBuffer);
}

String TimeManager::getCurrentDateString() const {
	if (!this->hasValidTime()) {
		return "No Date Available";
	}

	/// getEpochTime() already includes our timezone offset, so gmtime_r
	/// yields the local calendar date directly
	time_t t = this->ntpClient->getEpochTime();
	struct tm tm;
	gmtime_r(&t, &tm);

	char dateBuffer[16];
	strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &tm);
	return String(dateBuffer);
}

unsigned long TimeManager::getLastSyncTime() const {
	return this->lastSuccessfulSync;
}

unsigned long TimeManager::getTimeSinceLastSync() const {
	if (this->lastSuccessfulSync == 0) {
		return ULONG_MAX; /// We return max value if never synced
	}
	return millis() - this->lastSuccessfulSync;
}

bool TimeManager::needsSync() const {
	/// We need sync if we never synced or it's been too long
	return !this->timeValid || this->getTimeSinceLastSync() >= this->syncInterval;
}

unsigned long TimeManager::getSyncCount() const {
	return this->syncCount;
}

bool TimeManager::shouldAttemptSync() const {
	/// We retry faster while we have no time at all - this covers the 30s
	/// boot wait loop and a late WiFi connect - and fall back to the slower,
	/// server-friendly interval once we have a valid time to fall back on
	static constexpr unsigned long RETRY_INTERVAL_UNSYNCED_MS = 10000;
	static constexpr unsigned long RETRY_INTERVAL_SYNCED_MS = 60000;

	unsigned long minSyncInterval = this->timeValid ? RETRY_INTERVAL_SYNCED_MS : RETRY_INTERVAL_UNSYNCED_MS;
	return millis() - this->lastSyncAttempt >= minSyncInterval;
}

unsigned long TimeManager::getFailedSyncCount() const {
	return this->failedSyncCount;
}

float TimeManager::getSyncSuccessRate() const {
	unsigned long totalAttempts = this->syncCount + this->failedSyncCount;
	if (totalAttempts == 0) {
		return 1.0f; /// No attempts yet
	}
	return (float)this->syncCount / (float)totalAttempts;
}

bool TimeManager::attemptRecovery() {
	Serial.println("TimeManager: Attempting recovery...");

	/// We force an immediate sync attempt
	bool success = this->syncTime();

	if (success) {
		Serial.println("TimeManager: ✓ Recovery successful");
		return true;
	} else {
		Serial.println("TimeManager: ✗ Recovery failed");
		return false;
	}
}

bool TimeManager::isStarted() const {
	return this->started;
}