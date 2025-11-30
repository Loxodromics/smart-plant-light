///
/// SystemDiagnostics - System health monitoring and diagnostics
///
/// We track system uptime, component failures, recovery attempts,
/// and performance metrics to provide comprehensive health monitoring
/// and debugging information for the smart plant light controller.
///

#ifndef SYSTEMDIAGNOSTICS_H
#define SYSTEMDIAGNOSTICS_H

#include <Arduino.h>

/// Component identifier for failure tracking
enum class ComponentType {
	WiFi,
	Time,
	LightSensor,
	Relay,
	System
};

/// Failure record structure
struct FailureRecord {
	ComponentType component;
	unsigned long timestamp;
	const char* description;
};

/// Recovery attempt record
struct RecoveryRecord {
	ComponentType component;
	unsigned long timestamp;
	bool successful;
};

class SystemDiagnostics {
public:
	SystemDiagnostics();

	/// Initialize diagnostics system
	/// We check for crash markers and load persistent counters
	void begin();

	/// Record system startup
	/// We increment boot counter and check for unexpected reboots
	void recordStartup();

	/// Record a component failure
	/// We track failures for diagnostics and recovery decisions
	void recordFailure(ComponentType component, const char* description);

	/// Record a recovery attempt
	/// We track both successful and failed recovery attempts
	void recordRecovery(ComponentType component, bool successful);

	/// Update performance metrics
	/// We track WiFi signal strength trends
	void updateWiFiMetric(int rssi);

	/// Update sensor stability metric
	/// We track sensor reading variance
	void updateSensorMetric(float lux);

	/// Get system uptime in milliseconds
	[[nodiscard]] unsigned long getUptime() const;

	/// Get total boot count since first use
	[[nodiscard]] unsigned long getBootCount() const;

	/// Get failure count for specific component
	[[nodiscard]] unsigned long getFailureCount(ComponentType component) const;

	/// Get recovery success rate for component (0.0 to 1.0)
	[[nodiscard]] float getRecoveryRate(ComponentType component) const;

	/// Get average WiFi signal strength (dBm)
	[[nodiscard]] int getAverageWiFiSignal() const;

	/// Get sensor reading stability (standard deviation)
	[[nodiscard]] float getSensorStability() const;

	/// Check if system had unexpected reboot (crash)
	[[nodiscard]] bool hadUnexpectedReboot() const;

	/// Get last failure time for component
	[[nodiscard]] unsigned long getLastFailureTime(ComponentType component) const;

	/// Display comprehensive diagnostics report
	void displayReport() const;

	/// Get component name as string
	[[nodiscard]] const char* getComponentName(ComponentType component) const;

	/// Clear crash marker (for clean shutdown)
	void clearCrashMarker();

private:
	/// System tracking
	unsigned long bootTime;
	unsigned long bootCount;
	bool unexpectedReboot;

	/// Failure tracking (per component)
	unsigned long failureCount[5];  /// One per ComponentType
	unsigned long lastFailureTime[5];

	/// Recovery tracking
	unsigned long recoveryAttempts[5];
	unsigned long recoverySuccesses[5];

	/// Performance metrics (WiFi)
	static const int WIFI_METRIC_SIZE = 10;
	int wifiSignalBuffer[WIFI_METRIC_SIZE];
	int wifiBufferIndex;
	bool wifiBufferFull;

	/// Performance metrics (Sensor)
	static const int SENSOR_METRIC_SIZE = 20;
	float sensorReadingBuffer[SENSOR_METRIC_SIZE];
	int sensorBufferIndex;
	bool sensorBufferFull;

	/// Convert ComponentType to array index
	[[nodiscard]] int getComponentIndex(ComponentType component) const;

	/// Calculate average WiFi signal
	void calculateWiFiAverage();

	/// Calculate sensor stability (standard deviation)
	void calculateSensorStability();

	/// Load persistent counters from flash
	void loadPersistentData();

	/// Save persistent counters to flash
	void savePersistentData();

	/// Check for crash marker in RTC memory
	bool checkForCrash();

	/// Set crash marker in RTC memory
	void setCrashMarker();

	/// Clear crash marker
	void clearCrashMarker();
};

#endif /// SYSTEMDIAGNOSTICS_H
