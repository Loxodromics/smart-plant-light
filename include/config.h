/// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

/// Hardware Pin Definitions
#define RELAY_PIN 2
#define I2C_SDA_PIN 19
#define I2C_SCL_PIN 22

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
/// WiFi Configuration
#define WIFI_TIMEOUT_MS 10000

/// Time Configuration
#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_OFFSET_HOURS 2  /// Berlin = UTC+1
#define NTP_UPDATE_INTERVAL_MS 86400000  /// 24 hours

/// Plant Light Schedule (24-hour format)
#define LIGHT_START_HOUR 8
#define LIGHT_END_HOUR 23

/// Light Sensor Configuration
#define LIGHT_THRESHOLD_LUX 100.0  /// Turn on lights below this level
#define SENSOR_SAMPLES 5           /// Number of readings to average
#define CHECK_INTERVAL_MS 30000    /// Check every 30 seconds

/// Safety Configuration
#define MIN_SWITCH_INTERVAL_MS 60000  /// Minimum 1 minute between relay switches

#endif