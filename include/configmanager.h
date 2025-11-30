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

    /// Timezone offset from UTC
    int8_t timezoneOffsetHours;  /// -12 to +14

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

    /// Get current configuration (read-only)
    [[nodiscard]] const PlantLightConfig& getConfig() const;

    /// Update configuration (does not save automatically)
    void setWiFiCredentials(const char* ssid, const char* password);
    void setSchedule(uint8_t startHour, uint8_t endHour);
    void setLightThreshold(float thresholdLux);
    void setTimezone(int8_t offsetHours);

    /// Print current configuration to serial (for debugging)
    void printConfiguration() const;

private:
    Preferences preferences;
    PlantLightConfig config;

    /// Preferences namespace
    static constexpr const char* NAMESPACE = "plantlight";

    /// Current configuration version
    static constexpr uint32_t CONFIG_VERSION = 1;

    /// Validate individual configuration values
    [[nodiscard]] bool validateSchedule() const;
    [[nodiscard]] bool validateThreshold() const;
    [[nodiscard]] bool validateTimezone() const;
};

#endif
