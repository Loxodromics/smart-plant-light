///
/// TimeManager - Handles time synchronization and time-based logic
///
/// We use the ESP32's built-in SNTP client (lwIP), started via
/// configTzTime(), which runs entirely in the background - no polling
/// socket code or blocking forceUpdate() on the loop task. Local time
/// comes from a POSIX TZ string (setenv("TZ")/tzset()), so DST
/// transitions are handled by libc without a fixed hour offset.
///

#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <time.h>

class TimeManager {
public:
	explicit TimeManager(const char* ntpServer, const char* posixTz);

	/// Initialize the SNTP client (requires WiFi connection)
	/// We point lwIP's background SNTP client at our server and apply the
	/// POSIX TZ string. Safe to call once WiFi first connects; a second
	/// call is a no-op
	void begin();

	/// Check whether begin() has been called yet
	/// We use this to defer SNTP use until WiFi first connects
	[[nodiscard]] bool isStarted() const;

	/// Observe completed SNTP syncs (non-blocking)
	/// lwIP syncs in the background; we only count successful completions
	/// and log the resulting time. Call this regularly from loop()
	void update();

	/// Check if we have valid time (system clock past a plausible epoch)
	[[nodiscard]] bool hasValidTime() const;

	/// Get the full local time as a broken-down calendar struct
	/// Returns false when time is not valid (the struct then holds the
	/// unsynced ~1970 clock); DST-adjusted via localtime_r with our TZ string
	[[nodiscard]] bool getLocalTime(struct tm& out) const;

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

	/// Get time since last successful sync in milliseconds
	[[nodiscard]] unsigned long getTimeSinceLastSync() const;

	/// Get number of successful syncs since startup
	[[nodiscard]] unsigned long getSyncCount() const;

	/// Change the timezone at runtime, effective immediately
	/// We only update libc's TZ - SNTP keeps syncing UTC, so no resync
	/// or restart is needed for the change to take effect
	void setTimezone(const char* posixTz);

	/// Attempt recovery from time sync failure by restarting the SNTP
	/// client. Recovery is asynchronous: success can only be observed
	/// later via hasValidTime() once the restarted client completes a sync
	void attemptRecovery();

private:
	const char* ntpServer;
	char timezone[48];
	unsigned long lastSuccessfulSync;
	unsigned long syncCount;
	bool started;
};

#endif /// TIMEMANAGER_H
