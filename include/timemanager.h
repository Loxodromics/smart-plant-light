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
	/// Safe to call once WiFi first connects; a second call is a no-op
	void begin();

	/// Check whether begin() has been called yet
	/// We use this to defer NTP client use until WiFi first connects
	[[nodiscard]] bool isStarted() const;
	
	/// Update time from NTP server if needed
	/// We drive our own sync schedule (needsSync()/shouldAttemptSync()) rather
	/// than relying on the NTPClient library's internal update() - it force-syncs
	/// on every call while unsynced, which blocks the loop up to 1s per iteration
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

	/// Get current second (0-59)
	[[nodiscard]] int getCurrentSecond() const;

	/// Get current time as formatted string (HH:MM:SS)
	[[nodiscard]] String getCurrentTimeString() const;
	
	/// Get current date as formatted string (YYYY-MM-DD)
	[[nodiscard]] String getCurrentDateString() const;
	
	/// Get Unix timestamp of last successful sync
	[[nodiscard]] unsigned long getLastSyncTime() const;
	
	/// Get time since last successful sync in milliseconds
	[[nodiscard]] unsigned long getTimeSinceLastSync() const;
	
	/// Check if time data is stale and needs refresh
	[[nodiscard]] bool needsSync() const;
	
	/// Get number of successful syncs since startup
	[[nodiscard]] unsigned long getSyncCount() const;

	/// Get number of failed sync attempts
	[[nodiscard]] unsigned long getFailedSyncCount() const;

	/// Get sync success rate (0.0 to 1.0)
	[[nodiscard]] float getSyncSuccessRate() const;

	/// Attempt recovery from time sync failure
	/// We force immediate resync and clear stale state
	[[nodiscard]] bool attemptRecovery();

private:
	WiFiUDP ntpUDP;
	NTPClient* ntpClient;
	
	const char* ntpServer;
	int timezoneOffsetSeconds;
	unsigned long syncInterval;
	unsigned long lastSyncAttempt;
	unsigned long lastSuccessfulSync;
	unsigned long syncCount;
	unsigned long failedSyncCount;
	bool timeValid;
	bool started;

	/// Check if enough time has passed for next sync attempt
	[[nodiscard]] bool shouldAttemptSync() const;
};

#endif /// TIMEMANAGER_H