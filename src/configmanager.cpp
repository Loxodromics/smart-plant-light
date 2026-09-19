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
    bool needsMigrationSave = false;

    /// Load WiFi credentials
    this->preferences.getString("wifi.ssid", this->config.wifiSSID, sizeof(this->config.wifiSSID));
    this->preferences.getString("wifi.pass", this->config.wifiPassword, sizeof(this->config.wifiPassword));

    /// Load schedule
    this->config.lightStartHour = this->preferences.getUChar("sched.start", LIGHT_START_HOUR);
    this->config.lightEndHour = this->preferences.getUChar("sched.end", LIGHT_END_HOUR);
    /// v3 adds minute-level granularity; default 0 alone preserves HH:00 for
    /// pre-v3 stores, so no old value needs remapping
    this->config.lightStartMinute = this->preferences.getUChar("sched.start.min", LIGHT_START_MINUTE);
    this->config.lightEndMinute = this->preferences.getUChar("sched.end.min", LIGHT_END_MINUTE);

    /// Load sensor threshold
    this->config.lightThresholdLux = this->preferences.getFloat("sensor.thresh", LIGHT_THRESHOLD_LUX);

    /// Load hysteresis
    this->config.hysteresisLux = this->preferences.getFloat("sensor.hyster", 15.0);  /// Default 15 lux

    /// Load minimum relay switch interval
    this->config.minSwitchIntervalMs = this->preferences.getUInt("relay.minsw", MIN_SWITCH_INTERVAL_MS);

    /// Load timezone. The version field's first real use: v1 stored a UTC
    /// hour offset under "tz.offset", v2 stores a POSIX TZ string under
    /// "tz.posix". An old offset can't be mapped to a DST-capable zone, so
    /// a v1 config keeps the config.h default instead of migrating
    if (savedVersion < 2) {
        Serial.println("⚠️  ConfigManager: Timezone format changed in v2 - reset to config.h default");
        needsMigrationSave = true;
    } else {
        String timezone = this->preferences.getString("tz.posix", TIMEZONE_TZ);
        strncpy(this->config.timezone, timezone.c_str(), sizeof(this->config.timezone) - 1);
        this->config.timezone[sizeof(this->config.timezone) - 1] = '\0';
    }

    if (savedVersion < 3) {
        Serial.println("⚠️  ConfigManager: Schedule minute fields added in v3 - persisted as HH:00");
        needsMigrationSave = true;
    }

    Serial.println("✅ ConfigManager: Configuration loaded from flash");

    /// We write the migrated layout back once so the version bump sticks
    /// and the migration message doesn't repeat on every boot
    if (needsMigrationSave && this->saveConfiguration()) {
        this->config.version = CONFIG_VERSION;
    }
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
    this->preferences.putUChar("sched.start.min", this->config.lightStartMinute);
    this->preferences.putUChar("sched.end.min", this->config.lightEndMinute);

    /// Save sensor threshold
    this->preferences.putFloat("sensor.thresh", this->config.lightThresholdLux);

    /// Save hysteresis
    this->preferences.putFloat("sensor.hyster", this->config.hysteresisLux);

    /// Save minimum relay switch interval
    this->preferences.putUInt("relay.minsw", this->config.minSwitchIntervalMs);

    /// Save timezone
    this->preferences.putString("tz.posix", this->config.timezone);
    this->preferences.remove("tz.offset");  /// Cleanup from the pre-v2 offset scheme

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
    this->config.lightStartMinute = LIGHT_START_MINUTE;
    this->config.lightEndHour = LIGHT_END_HOUR;
    this->config.lightEndMinute = LIGHT_END_MINUTE;

    /// Set default threshold
    this->config.lightThresholdLux = LIGHT_THRESHOLD_LUX;

    /// Set default hysteresis
    this->config.hysteresisLux = 15.0;  /// 15 lux dead band

    /// Set default minimum relay switch interval
    this->config.minSwitchIntervalMs = MIN_SWITCH_INTERVAL_MS;

    /// Set default timezone
    strncpy(this->config.timezone, TIMEZONE_TZ, sizeof(this->config.timezone) - 1);
    this->config.timezone[sizeof(this->config.timezone) - 1] = '\0';

    /// Set version
    this->config.version = CONFIG_VERSION;
}

bool ConfigManager::isValid() const {
    return validate(this->config);
}

bool ConfigManager::validate(const PlantLightConfig& config) {
    return validateSchedule(config) && validateThreshold(config) && validateHysteresis(config) &&
           validateMinSwitchInterval(config) && validateTimezone(config);
}

const PlantLightConfig& ConfigManager::getConfig() const {
    return this->config;
}

void ConfigManager::setConfig(const PlantLightConfig& config) {
    this->config = config;
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

void ConfigManager::setSchedule(uint8_t startHour, uint8_t startMinute, uint8_t endHour, uint8_t endMinute) {
    this->config.lightStartHour = startHour;
    this->config.lightStartMinute = startMinute;
    this->config.lightEndHour = endHour;
    this->config.lightEndMinute = endMinute;
}

void ConfigManager::setLightThreshold(float thresholdLux) {
    this->config.lightThresholdLux = thresholdLux;
}

void ConfigManager::setHysteresis(float hysteresisLux) {
    this->config.hysteresisLux = hysteresisLux;
}

void ConfigManager::setMinSwitchInterval(uint32_t intervalMs) {
    this->config.minSwitchIntervalMs = intervalMs;
}

void ConfigManager::setTimezone(const char* posixTz) {
    if (posixTz != nullptr) {
        strncpy(this->config.timezone, posixTz, sizeof(this->config.timezone) - 1);
        this->config.timezone[sizeof(this->config.timezone) - 1] = '\0';
    }
}

void ConfigManager::printConfiguration() const {
    Serial.println("📋 Current Configuration:");
    Serial.printf("  WiFi SSID: %s\n", this->config.wifiSSID);
    Serial.printf("  WiFi Password: %s\n", strlen(this->config.wifiPassword) > 0 ? "********" : "(empty)");
    Serial.printf("  Schedule: %02d:%02d - %02d:%02d\n",
        this->config.lightStartHour, this->config.lightStartMinute,
        this->config.lightEndHour, this->config.lightEndMinute);
    Serial.printf("  Light Threshold: %.1f lux\n", this->config.lightThresholdLux);
    Serial.printf("  Hysteresis: %.1f lux\n", this->config.hysteresisLux);
    Serial.printf("  Min switch interval: %u ms\n", this->config.minSwitchIntervalMs);
    Serial.printf("  Timezone: %s\n", this->config.timezone);
    Serial.printf("  Config Version: %u\n", this->config.version);
}

bool ConfigManager::validateSchedule(const PlantLightConfig& config) {
    /// Hours must be in valid range 0-23, minutes 0-59
    if (config.lightStartHour > 23 || config.lightEndHour > 23 ||
        config.lightStartMinute > 59 || config.lightEndMinute > 59) {
        Serial.println("❌ Invalid schedule: Hours must be 0-23, minutes 0-59");
        return false;
    }

    /// Start and end can be equal (24-hour operation) or different
    /// We support overnight schedules (e.g., 22:00 - 06:00)
    return true;
}

bool ConfigManager::validateThreshold(const PlantLightConfig& config) {
    /// Threshold must be positive and within reasonable range
    /// VEML7700 max reading is ~120,000 lux
    if (config.lightThresholdLux < 0.0 || config.lightThresholdLux > 10000.0) {
        Serial.printf("❌ Invalid threshold: %.1f lux (must be 0-10000)\n", config.lightThresholdLux);
        return false;
    }
    return true;
}

bool ConfigManager::validateHysteresis(const PlantLightConfig& config) {
    /// Hysteresis must be non-negative and reasonable
    /// Maximum 100 lux is a reasonable upper bound
    if (config.hysteresisLux < 0.0 || config.hysteresisLux > 100.0) {
        Serial.printf("❌ Invalid hysteresis: %.1f lux (must be 0-100)\n", config.hysteresisLux);
        return false;
    }
    return true;
}

bool ConfigManager::validateMinSwitchInterval(const PlantLightConfig& config) {
    /// We require a nonzero floor to protect the relay hardware even though
    /// this setting is primarily about not annoying the user with clicking -
    /// 1s-10min covers everything from "barely any protection" to "very lazy"
    if (config.minSwitchIntervalMs < 1000 || config.minSwitchIntervalMs > 600000) {
        Serial.printf("❌ Invalid min switch interval: %u ms (must be 1000-600000)\n", config.minSwitchIntervalMs);
        return false;
    }
    return true;
}

bool ConfigManager::validateTimezone(const PlantLightConfig& config) {
    /// We only check for a non-empty string that fits the buffer - POSIX TZ
    /// syntax isn't validated here; an unparsable string makes libc fall
    /// back to UTC, which the web UI hint documents
    size_t len = strlen(config.timezone);
    if (len == 0 || len >= sizeof(config.timezone)) {
        Serial.println("❌ Invalid timezone: must be a non-empty POSIX TZ string (max 47 chars)");
        return false;
    }
    return true;
}
