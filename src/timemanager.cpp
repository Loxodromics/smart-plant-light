///
/// TimeManager Implementation
/// 
/// We implement reliable NTP time synchronization for the plant
/// light controller. This ensures accurate time-based scheduling
/// even if the device loses power or WiFi connection temporarily.
///

#include "timemanager.h"
#include "config.h"

TimeManager::TimeManager(const char* ntpServer, int timezoneOffsetHours)
	: ntpServer(ntpServer)
	, timezoneOffsetSeconds(timezoneOffsetHours * 3600)
	, syncInterval(NTP_UPDATE_INTERVAL_MS)
	, lastSyncAttempt(0)
	, lastSuccessfulSync(0)
	, syncCount(0)
	, timeValid(false)
{
	/// We create the NTP client with our timezone offset
	this->ntpClient = new NTPClient(this->ntpUDP, this->ntpServer, this->timezoneOffsetSeconds);
}

TimeManager::~TimeManager() {
	/// We clean up dynamically allocated NTP client
	delete this->ntpClient;
}

void TimeManager::begin() {
	/// We initialize the NTP client
	this->ntpClient->begin();
	
	/// We set update interval (how often client fetches internally)
	this->ntpClient->setUpdateInterval(this->syncInterval);
	
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
	/// We check if it's time for a sync
	if (this->needsSync() && this->shouldAttemptSync()) {
		Serial.println("TimeManager: Performing scheduled sync...");
		bool syncResult = this->syncTime();
		if (!syncResult) {
			Serial.println("TimeManager: Scheduled sync failed, will retry later");
		}
	}
	
	/// We update the NTP client's internal state
	this->ntpClient->update();
}

bool TimeManager::syncTime() {
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
	
	/// We format the date manually since getFormattedDate() is not available
	/// We get the epoch time and format it
	unsigned long epochTime = this->ntpClient->getEpochTime();
	
	/// We calculate date components from epoch time
	/// This is a simplified calculation for basic date display
	unsigned long daysSinceEpoch = epochTime / 86400;
	unsigned long year = 1970;
	unsigned long month = 1;
	unsigned long day = 1;
	
	/// We add approximate years (accounting for leap years)
	while (daysSinceEpoch >= 365) {
		bool isLeapYear = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
		unsigned long daysThisYear = isLeapYear ? 366 : 365;
		
		if (daysSinceEpoch >= daysThisYear) {
			daysSinceEpoch -= daysThisYear;
			year++;
		} else {
			break;
		}
	}
	
	/// We format as simple date string
	char dateBuffer[32];
	snprintf(dateBuffer, sizeof(dateBuffer), "%04lu-%02lu-%02lu (%s)", 
			year, month, day + daysSinceEpoch, this->ntpClient->getFormattedTime().c_str());
	return String(dateBuffer);
}

bool TimeManager::isTimeInRange(int startHour, int endHour) const {
	if (!this->hasValidTime()) {
		return false; /// We can't make time decisions without valid time
	}
	
	int currentHour = this->getCurrentHour();
	return this->isTimeInRangeWithDayBoundary(startHour, endHour, currentHour);
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
	/// We don't attempt sync too frequently to avoid overloading NTP servers
	const unsigned long minSyncInterval = 60000; /// Minimum 1 minute between attempts
	return millis() - this->lastSyncAttempt >= minSyncInterval;
}

bool TimeManager::isTimeInRangeWithDayBoundary(int startHour, int endHour, int currentHour) const {
	/// We handle normal ranges (e.g., 6 to 22)
	if (startHour <= endHour) {
		return currentHour >= startHour && currentHour < endHour;
	}
	
	/// We handle ranges that cross midnight (e.g., 22 to 6)
	/// This means lights are on from 22:00 to 06:00 (overnight)
	return currentHour >= startHour || currentHour < endHour;
}