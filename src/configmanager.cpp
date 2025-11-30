/// src/configmanager.cpp
#include "configmanager.h"
#include <string.h>

ConfigManager::ConfigManager() {
    /// Initialize with default values from config.h
    resetToDefaults();
}

ConfigManager::~ConfigManager() {
    /// Close preferences on destruction
    this->preferences.end();
}

bool ConfigManager::begin() {
    Serial.println("🔧 ConfigManager: Initializing...");

    /// Open preferences in read-write mode
    if (!this->preferences.begin(NAMESPACE, false)) {
        Serial.println("❌ ConfigManager: Failed to open preferences");
        return false;
    }

    /// Try to load saved configuration
    loadConfiguration();

    /// Validate loaded configuration
    if (!isValid()) {
        Serial.println("⚠️  ConfigManager: Invalid configuration, using defaults");
        resetToDefaults();
        saveConfiguration();
    }

    printConfiguration();
    return true;
}

void ConfigManager::loadConfiguration() {
    /// Check if we have a saved configuration
    uint32_t savedVersion = this->preferences.getUInt("version", 0);

    if (savedVersion == 0) {
        Serial.println("ℹ️  ConfigManager: No saved configuration, using defaults");
        resetToDefaults();
        return;
    }

    Serial.println("📂 ConfigManager: Loading saved configuration...");
    this->config.version = savedVersion;

    /// Load WiFi credentials
    this->preferences.getString("wifi.ssid", this->config.wifiSSID, sizeof(this->config.wifiSSID));
    this->preferences.getString("wifi.pass", this->config.wifiPassword, sizeof(this->config.wifiPassword));

    /// Load schedule
    this->config.lightStartHour = this->preferences.getUChar("sched.start", LIGHT_START_HOUR);
    this->config.lightEndHour = this->preferences.getUChar("sched.end", LIGHT_END_HOUR);

    /// Load sensor threshold
    this->config.lightThresholdLux = this->preferences.getFloat("sensor.thresh", LIGHT_THRESHOLD_LUX);

    /// Load hysteresis
    this->config.hysteresisLux = this->preferences.getFloat("sensor.hyster", 15.0);  /// Default 15 lux

    /// Load timezone
    this->config.timezoneOffsetHours = this->preferences.getChar("tz.offset", TIMEZONE_OFFSET_HOURS);

    Serial.println("✅ ConfigManager: Configuration loaded from flash");
}

bool ConfigManager::saveConfiguration() {
    /// Validate before saving
    if (!isValid()) {
        Serial.println("❌ ConfigManager: Cannot save invalid configuration");
        return false;
    }

    Serial.println("💾 ConfigManager: Saving configuration...");

    /// Save version
    this->preferences.putUInt("version", CONFIG_VERSION);

    /// Save WiFi credentials
    this->preferences.putString("wifi.ssid", this->config.wifiSSID);
    this->preferences.putString("wifi.pass", this->config.wifiPassword);

    /// Save schedule
    this->preferences.putUChar("sched.start", this->config.lightStartHour);
    this->preferences.putUChar("sched.end", this->config.lightEndHour);

    /// Save sensor threshold
    this->preferences.putFloat("sensor.thresh", this->config.lightThresholdLux);

    /// Save hysteresis
    this->preferences.putFloat("sensor.hyster", this->config.hysteresisLux);

    /// Save timezone
    this->preferences.putChar("tz.offset", this->config.timezoneOffsetHours);

    Serial.println("✅ ConfigManager: Configuration saved to flash");
    return true;
}

void ConfigManager::resetToDefaults() {
    Serial.println("🔄 ConfigManager: Resetting to defaults from config.h");

    /// Set default WiFi credentials from config.h
    strncpy(this->config.wifiSSID, WIFI_SSID, sizeof(this->config.wifiSSID) - 1);
    this->config.wifiSSID[sizeof(this->config.wifiSSID) - 1] = '\0';

    strncpy(this->config.wifiPassword, WIFI_PASSWORD, sizeof(this->config.wifiPassword) - 1);
    this->config.wifiPassword[sizeof(this->config.wifiPassword) - 1] = '\0';

    /// Set default schedule
    this->config.lightStartHour = LIGHT_START_HOUR;
    this->config.lightEndHour = LIGHT_END_HOUR;

    /// Set default threshold
    this->config.lightThresholdLux = LIGHT_THRESHOLD_LUX;

    /// Set default hysteresis
    this->config.hysteresisLux = 15.0;  /// 15 lux dead band

    /// Set default timezone
    this->config.timezoneOffsetHours = TIMEZONE_OFFSET_HOURS;

    /// Set version
    this->config.version = CONFIG_VERSION;
}

bool ConfigManager::isValid() const {
    return validateSchedule() && validateThreshold() && validateHysteresis() && validateTimezone();
}

const PlantLightConfig& ConfigManager::getConfig() const {
    return this->config;
}

void ConfigManager::setWiFiCredentials(const char* ssid, const char* password) {
    if (ssid != nullptr) {
        strncpy(this->config.wifiSSID, ssid, sizeof(this->config.wifiSSID) - 1);
        this->config.wifiSSID[sizeof(this->config.wifiSSID) - 1] = '\0';
    }

    if (password != nullptr) {
        strncpy(this->config.wifiPassword, password, sizeof(this->config.wifiPassword) - 1);
        this->config.wifiPassword[sizeof(this->config.wifiPassword) - 1] = '\0';
    }
}

void ConfigManager::setSchedule(uint8_t startHour, uint8_t endHour) {
    this->config.lightStartHour = startHour;
    this->config.lightEndHour = endHour;
}

void ConfigManager::setLightThreshold(float thresholdLux) {
    this->config.lightThresholdLux = thresholdLux;
}

void ConfigManager::setHysteresis(float hysteresisLux) {
    this->config.hysteresisLux = hysteresisLux;
}

void ConfigManager::setTimezone(int8_t offsetHours) {
    this->config.timezoneOffsetHours = offsetHours;
}

void ConfigManager::printConfiguration() const {
    Serial.println("📋 Current Configuration:");
    Serial.printf("  WiFi SSID: %s\n", this->config.wifiSSID);
    Serial.printf("  WiFi Password: %s\n", strlen(this->config.wifiPassword) > 0 ? "********" : "(empty)");
    Serial.printf("  Schedule: %02d:00 - %02d:00\n", this->config.lightStartHour, this->config.lightEndHour);
    Serial.printf("  Light Threshold: %.1f lux\n", this->config.lightThresholdLux);
    Serial.printf("  Hysteresis: %.1f lux\n", this->config.hysteresisLux);
    Serial.printf("  Timezone: UTC%+d\n", this->config.timezoneOffsetHours);
    Serial.printf("  Config Version: %u\n", this->config.version);
}

bool ConfigManager::validateSchedule() const {
    /// Hours must be in valid range 0-23
    if (this->config.lightStartHour > 23 || this->config.lightEndHour > 23) {
        Serial.println("❌ Invalid schedule: Hours must be 0-23");
        return false;
    }

    /// Start and end can be equal (24-hour operation) or different
    /// We support overnight schedules (e.g., 22:00 - 06:00)
    return true;
}

bool ConfigManager::validateThreshold() const {
    /// Threshold must be positive and within reasonable range
    /// VEML7700 max reading is ~120,000 lux
    if (this->config.lightThresholdLux < 0.0 || this->config.lightThresholdLux > 10000.0) {
        Serial.printf("❌ Invalid threshold: %.1f lux (must be 0-10000)\n", this->config.lightThresholdLux);
        return false;
    }
    return true;
}

bool ConfigManager::validateHysteresis() const {
    /// Hysteresis must be non-negative and reasonable
    /// Maximum 100 lux is a reasonable upper bound
    if (this->config.hysteresisLux < 0.0 || this->config.hysteresisLux > 100.0) {
        Serial.printf("❌ Invalid hysteresis: %.1f lux (must be 0-100)\n", this->config.hysteresisLux);
        return false;
    }
    return true;
}

bool ConfigManager::validateTimezone() const{
    /// Timezone offset must be within valid range
    /// UTC-12 (Baker Island) to UTC+14 (Kiribati)
    if (this->config.timezoneOffsetHours < -12 || this->config.timezoneOffsetHours > 14) {
        Serial.printf("❌ Invalid timezone: UTC%+d (must be -12 to +14)\n", this->config.timezoneOffsetHours);
        return false;
    }
    return true;
}
