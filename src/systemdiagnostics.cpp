///
/// SystemDiagnostics Implementation
///
/// We implement comprehensive system monitoring to track health,
/// failures, and recovery across all components. This provides
/// visibility into system reliability and helps with debugging.
///

#include "systemdiagnostics.h"
#include <Preferences.h>
#include <esp_system.h>
#include <math.h>

/// RTC memory for crash detection (survives soft reboots but not power cycles)
RTC_DATA_ATTR bool rtcCrashMarker = false;

SystemDiagnostics::SystemDiagnostics()
	: bootTime(0)
	, bootCount(0)
	, unexpectedReboot(false)
	, wifiBufferIndex(0)
	, wifiBufferFull(false)
	, sensorBufferIndex(0)
	, sensorBufferFull(false)
{
	/// We initialize all counters to zero
	for (int i = 0; i < 5; i++) {
		this->failureCount[i] = 0;
		this->lastFailureTime[i] = 0;
		this->recoveryAttempts[i] = 0;
		this->recoverySuccesses[i] = 0;
	}

	/// We initialize metric buffers
	for (int i = 0; i < WIFI_METRIC_SIZE; i++) {
		this->wifiSignalBuffer[i] = 0;
	}
	for (int i = 0; i < SENSOR_METRIC_SIZE; i++) {
		this->sensorReadingBuffer[i] = 0.0f;
	}
}

void SystemDiagnostics::begin() {
	Serial.println("SystemDiagnostics: Initializing...");

	/// We record the boot time
	this->bootTime = millis();

	/// We check for crash marker
	this->unexpectedReboot = this->checkForCrash();

	/// We load persistent data
	this->loadPersistentData();

	/// We increment boot count
	this->bootCount++;
	this->savePersistentData();

	/// We set crash marker (cleared on clean shutdown)
	this->setCrashMarker();

	Serial.print("SystemDiagnostics: Boot #");
	Serial.println(this->bootCount);

	if (this->unexpectedReboot) {
		Serial.println("SystemDiagnostics: ⚠ Unexpected reboot detected (possible crash)");
	}

	Serial.println("SystemDiagnostics: ✓ Initialized");
}

void SystemDiagnostics::recordStartup() {
	Serial.println("SystemDiagnostics: System startup recorded");
}

void SystemDiagnostics::recordFailure(ComponentType component, const char* description) {
	int idx = this->getComponentIndex(component);

	this->failureCount[idx]++;
	this->lastFailureTime[idx] = millis();

	Serial.print("SystemDiagnostics: Failure recorded - ");
	Serial.print(this->getComponentName(component));
	Serial.print(": ");
	Serial.println(description);

	/// We save updated failure counts
	this->savePersistentData();
}

void SystemDiagnostics::recordRecovery(ComponentType component, bool successful) {
	int idx = this->getComponentIndex(component);

	this->recoveryAttempts[idx]++;
	if (successful) {
		this->recoverySuccesses[idx]++;
	}

	Serial.print("SystemDiagnostics: Recovery ");
	Serial.print(successful ? "SUCCESSFUL" : "FAILED");
	Serial.print(" - ");
	Serial.println(this->getComponentName(component));

	/// We save updated recovery stats
	this->savePersistentData();
}

void SystemDiagnostics::updateWiFiMetric(int rssi) {
	/// We add to circular buffer
	this->wifiSignalBuffer[this->wifiBufferIndex] = rssi;
	this->wifiBufferIndex++;

	if (this->wifiBufferIndex >= WIFI_METRIC_SIZE) {
		this->wifiBufferIndex = 0;
		this->wifiBufferFull = true;
	}
}

void SystemDiagnostics::updateSensorMetric(float lux) {
	/// We add to circular buffer
	this->sensorReadingBuffer[this->sensorBufferIndex] = lux;
	this->sensorBufferIndex++;

	if (this->sensorBufferIndex >= SENSOR_METRIC_SIZE) {
		this->sensorBufferIndex = 0;
		this->sensorBufferFull = true;
	}
}

unsigned long SystemDiagnostics::getUptime() const {
	return millis() - this->bootTime;
}

unsigned long SystemDiagnostics::getBootCount() const {
	return this->bootCount;
}

unsigned long SystemDiagnostics::getFailureCount(ComponentType component) const {
	int idx = this->getComponentIndex(component);
	return this->failureCount[idx];
}

float SystemDiagnostics::getRecoveryRate(ComponentType component) const {
	int idx = this->getComponentIndex(component);

	if (this->recoveryAttempts[idx] == 0) {
		return 1.0f; /// No failures = perfect rate
	}

	return (float)this->recoverySuccesses[idx] / (float)this->recoveryAttempts[idx];
}

int SystemDiagnostics::getAverageWiFiSignal() const {
	if (!this->wifiBufferFull && this->wifiBufferIndex == 0) {
		return 0; /// No data
	}

	int sum = 0;
	int count = this->wifiBufferFull ? WIFI_METRIC_SIZE : this->wifiBufferIndex;

	for (int i = 0; i < count; i++) {
		sum += this->wifiSignalBuffer[i];
	}

	return sum / count;
}

float SystemDiagnostics::getSensorStability() const {
	if (!this->sensorBufferFull && this->sensorBufferIndex < 2) {
		return 0.0f; /// Need at least 2 samples
	}

	int count = this->sensorBufferFull ? SENSOR_METRIC_SIZE : this->sensorBufferIndex;

	/// We calculate mean
	float sum = 0.0f;
	for (int i = 0; i < count; i++) {
		sum += this->sensorReadingBuffer[i];
	}
	float mean = sum / count;

	/// We calculate standard deviation
	float varianceSum = 0.0f;
	for (int i = 0; i < count; i++) {
		float diff = this->sensorReadingBuffer[i] - mean;
		varianceSum += diff * diff;
	}

	return sqrt(varianceSum / count);
}

bool SystemDiagnostics::hadUnexpectedReboot() const {
	return this->unexpectedReboot;
}

unsigned long SystemDiagnostics::getLastFailureTime(ComponentType component) const {
	int idx = this->getComponentIndex(component);
	return this->lastFailureTime[idx];
}

void SystemDiagnostics::displayReport() const {
	Serial.println("━━━ System Diagnostics Report ━━━");

	/// We display uptime
	unsigned long uptimeSeconds = this->getUptime() / 1000;
	unsigned long days = uptimeSeconds / 86400;
	unsigned long hours = (uptimeSeconds % 86400) / 3600;
	unsigned long minutes = (uptimeSeconds % 3600) / 60;
	unsigned long seconds = uptimeSeconds % 60;

	Serial.print("⏱ Uptime: ");
	if (days > 0) {
		Serial.print(days);
		Serial.print("d ");
	}
	Serial.print(hours);
	Serial.print("h ");
	Serial.print(minutes);
	Serial.print("m ");
	Serial.print(seconds);
	Serial.println("s");

	Serial.print("🔄 Boot count: ");
	Serial.println(this->bootCount);

	if (this->unexpectedReboot) {
		Serial.println("⚠️  Last boot was unexpected (possible crash)");
	}

	Serial.println();
	Serial.println("Component Health:");

	/// We display component statistics
	for (int i = 0; i < 5; i++) {
		ComponentType comp = static_cast<ComponentType>(i);
		const char* name = this->getComponentName(comp);

		Serial.print("  ");
		Serial.print(name);
		Serial.print(": ");

		if (this->failureCount[i] == 0) {
			Serial.println("✅ No failures");
		} else {
			Serial.print("⚠️  ");
			Serial.print(this->failureCount[i]);
			Serial.print(" failures");

			if (this->recoveryAttempts[i] > 0) {
				float rate = this->getRecoveryRate(comp);
				Serial.print(", ");
				Serial.print((int)(rate * 100));
				Serial.print("% recovery rate (");
				Serial.print(this->recoverySuccesses[i]);
				Serial.print("/");
				Serial.print(this->recoveryAttempts[i]);
				Serial.print(")");
			}

			Serial.println();
		}
	}

	/// We display performance metrics
	Serial.println();
	Serial.println("Performance Metrics:");

	int avgWiFi = this->getAverageWiFiSignal();
	if (avgWiFi != 0) {
		Serial.print("  📡 WiFi signal (avg): ");
		Serial.print(avgWiFi);
		Serial.println(" dBm");
	}

	float sensorStability = this->getSensorStability();
	if (sensorStability > 0.0f) {
		Serial.print("  💡 Sensor stability (σ): ");
		Serial.print(sensorStability, 2);
		Serial.println(" lux");
	}
}

const char* SystemDiagnostics::getComponentName(ComponentType component) const {
	switch (component) {
		case ComponentType::WiFi: return "WiFi";
		case ComponentType::Time: return "Time";
		case ComponentType::LightSensor: return "LightSensor";
		case ComponentType::Relay: return "Relay";
		case ComponentType::System: return "System";
		default: return "Unknown";
	}
}

int SystemDiagnostics::getComponentIndex(ComponentType component) const {
	return static_cast<int>(component);
}

void SystemDiagnostics::loadPersistentData() {
	Preferences prefs;
	prefs.begin("diagnostics", true); /// Read-only

	this->bootCount = prefs.getULong("bootCount", 0);

	/// We load failure counts
	for (int i = 0; i < 5; i++) {
		char key[16];
		snprintf(key, sizeof(key), "fail_%d", i);
		this->failureCount[i] = prefs.getULong(key, 0);

		snprintf(key, sizeof(key), "rec_att_%d", i);
		this->recoveryAttempts[i] = prefs.getULong(key, 0);

		snprintf(key, sizeof(key), "rec_suc_%d", i);
		this->recoverySuccesses[i] = prefs.getULong(key, 0);
	}

	prefs.end();
}

void SystemDiagnostics::savePersistentData() {
	Preferences prefs;
	prefs.begin("diagnostics", false); /// Read-write

	prefs.putULong("bootCount", this->bootCount);

	/// We save failure and recovery counts
	for (int i = 0; i < 5; i++) {
		char key[16];
		snprintf(key, sizeof(key), "fail_%d", i);
		prefs.putULong(key, this->failureCount[i]);

		snprintf(key, sizeof(key), "rec_att_%d", i);
		prefs.putULong(key, this->recoveryAttempts[i]);

		snprintf(key, sizeof(key), "rec_suc_%d", i);
		prefs.putULong(key, this->recoverySuccesses[i]);
	}

	prefs.end();
}

bool SystemDiagnostics::checkForCrash() {
	/// We check the RTC memory marker
	/// If it's set, we had a crash (didn't clean shutdown)
	return rtcCrashMarker;
}

void SystemDiagnostics::setCrashMarker() {
	/// We set the crash marker
	/// This will be cleared on clean shutdown
	rtcCrashMarker = true;
}

void SystemDiagnostics::clearCrashMarker() {
	/// We clear the crash marker on clean shutdown
	rtcCrashMarker = false;
	Serial.println("SystemDiagnostics: Crash marker cleared (clean shutdown)");
}
