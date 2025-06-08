///
/// TimeManager - Handles NTP time synchronization and time-based logic
/// 
/// We implement reliable time synchronization using NTP servers
/// and provide time-based functionality for the plant light schedule.
/// The manager handles timezone offsets and provides easy access
/// to current time information.
///

#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

class TimeManager {
public:
	explicit TimeManager(const char* ntpServer, int timezoneOffsetHours);
	~TimeManager();
	
	/// Initialize the NTP client (requires WiFi connection)
	/// We set up the NTP client with server and timezone configuration
	void begin();
	
	/// Update time from NTP server if needed
	/// We check if it's time for a sync and perform it if necessary
	void update();
	
	/// Force immediate time synchronization
	/// Returns true if sync was successful, false if failed
	[[nodiscard]] bool syncTime();
	
	/// Check if we have valid time (have synced at least once)
	[[nodiscard]] bool hasValidTime() const;
	
	/// Get current hour in 24-hour format (0-23)
	[[nodiscard]] int getCurrentHour() const;
	
	/// Get current minute (0-59)
	[[nodiscard]] int getCurrentMinute() const;
	
	/// Get current time as formatted string (HH:MM:SS)
	[[nodiscard]] String getCurrentTimeString() const;
	
	/// Get current date as formatted string (YYYY-MM-DD)
	[[nodiscard]] String getCurrentDateString() const;
	
	/// Check if current time is within specified hour range
	/// We use this for plant light scheduling logic
	[[nodiscard]] bool isTimeInRange(int startHour, int endHour) const;
	
	/// Get Unix timestamp of last successful sync
	[[nodiscard]] unsigned long getLastSyncTime() const;
	
	/// Get time since last successful sync in milliseconds
	[[nodiscard]] unsigned long getTimeSinceLastSync() const;
	
	/// Check if time data is stale and needs refresh
	[[nodiscard]] bool needsSync() const;
	
	/// Get number of successful syncs since startup
	[[nodiscard]] unsigned long getSyncCount() const;

private:
	WiFiUDP ntpUDP;
	NTPClient* ntpClient;
	
	const char* ntpServer;
	int timezoneOffsetSeconds;
	unsigned long syncInterval;
	unsigned long lastSyncAttempt;
	unsigned long lastSuccessfulSync;
	unsigned long syncCount;
	bool timeValid;
	
	/// Check if enough time has passed for next sync attempt
	[[nodiscard]] bool shouldAttemptSync() const;
	
	/// Handle day boundary crossing for time ranges
	/// We need special logic for ranges that cross midnight
	[[nodiscard]] bool isTimeInRangeWithDayBoundary(int startHour, int endHour, int currentHour) const;
};

#endif /// TIMEMANAGER_H