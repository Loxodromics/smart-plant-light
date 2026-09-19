/// include/configmanager.h
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <Preferences.h>
#include "config.h"

/// Configuration structure for plant light system
struct PlantLightConfig {
    /// WiFi credentials
    char wifiSSID[32];
    char wifiPassword[64];

    /// Light schedule (24-hour format)
    uint8_t lightStartHour;  /// 0-23
    uint8_t lightEndHour;    /// 0-23

    /// Light sensor threshold
    float lightThresholdLux;

    /// Applied symmetrically: lights turn on below threshold - h, off above threshold + h
    float hysteresisLux;  /// 0-100 lux

    /// Minimum time between relay switches, regardless of decision logic
    uint32_t minSwitchIntervalMs;  /// 1000-600000 ms

    /// POSIX TZ string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3"); an unparsable
    /// string makes libc fall back to UTC
    char timezone[48];

    /// Configuration version for future migration
    uint32_t version;
};

/// Manages configuration persistence using ESP32 Preferences API
/// We store all user-configurable settings in flash memory so they
/// survive reboots and power cycles without requiring recompilation
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    /// Initialize the configuration manager and load saved settings
    /// Returns true if configuration was loaded, false if using defaults
    [[nodiscard]] bool begin();

    /// Load configuration from flash memory
    /// Falls back to config.h defaults if no saved configuration exists
    void loadConfiguration();

    /// Save current configuration to flash memory
    /// Returns true if save was successful
    [[nodiscard]] bool saveConfiguration();

    /// Reset configuration to defaults from config.h
    void resetToDefaults();

    /// Validate configuration values are within acceptable ranges
    /// Returns true if configuration is valid
    [[nodiscard]] bool isValid() const;

    /// Validate an arbitrary candidate configuration without touching the
    /// live config - lets callers (e.g. the web server) check a candidate
    /// before committing it
    [[nodiscard]] static bool validate(const PlantLightConfig& config);

    /// Get current configuration (read-only)
    [[nodiscard]] const PlantLightConfig& getConfig() const;

    /// Replace the whole configuration in one assignment
    void setConfig(const PlantLightConfig& config);

    /// Update configuration (does not save automatically)
    void setWiFiCredentials(const char* ssid, const char* password);
    void setSchedule(uint8_t startHour, uint8_t endHour);
    void setLightThreshold(float thresholdLux);
    void setHysteresis(float hysteresisLux);
    void setMinSwitchInterval(uint32_t intervalMs);
    void setTimezone(const char* posixTz);

    /// Print current configuration to serial (for debugging)
    void printConfiguration() const;

private:
    Preferences preferences;
    PlantLightConfig config;

    /// Preferences namespace
    static constexpr const char* NAMESPACE = "plantlight";

    /// Current configuration version - bumped when the stored layout changes
    /// in a way loadConfiguration() must handle. First real use: v1 stored a
    /// UTC hour offset ("tz.offset"), v2 stores a POSIX TZ string ("tz.posix")
    static constexpr uint32_t CONFIG_VERSION = 2;

    /// Validate individual configuration values
    [[nodiscard]] static bool validateSchedule(const PlantLightConfig& config);
    [[nodiscard]] static bool validateThreshold(const PlantLightConfig& config);
    [[nodiscard]] static bool validateHysteresis(const PlantLightConfig& config);
    [[nodiscard]] static bool validateMinSwitchInterval(const PlantLightConfig& config);
    [[nodiscard]] static bool validateTimezone(const PlantLightConfig& config);
};

#endif
