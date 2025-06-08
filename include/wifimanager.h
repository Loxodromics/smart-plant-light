///
/// WiFiManager - Handles WiFi connection and reconnection logic
/// 
/// We implement robust WiFi connectivity with automatic reconnection
/// to ensure the system stays connected for NTP time synchronization.
/// The manager handles connection failures gracefully and provides
/// status information for debugging.
///

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFi.h>

enum class WiFiStatus {
	Disconnected,
	Connecting,
	Connected,
	Failed
};

class WiFiManager {
public:
	WiFiManager(const char* ssid, const char* password);
	
	/// Initialize WiFi and attempt initial connection
	/// We set up WiFi parameters and start the connection process
	void begin();
	
	/// Check and maintain WiFi connection
	/// We monitor connection status and attempt reconnection if needed
	void update();
	
	/// Attempt to connect to WiFi network
	/// Returns true if connection successful, false if failed or timeout
	[[nodiscard]] bool connect();
	
	/// Get current WiFi connection status
	[[nodiscard]] WiFiStatus getStatus() const;
	
	/// Check if WiFi is currently connected
	[[nodiscard]] bool isConnected() const;
	
	/// Get WiFi signal strength in dBm
	/// We use this to assess connection quality
	[[nodiscard]] int getSignalStrength() const;
	
	/// Get local IP address as string
	/// We provide this for debugging and status display
	[[nodiscard]] String getLocalIP() const;
	
	/// Get time since last successful connection in milliseconds
	[[nodiscard]] unsigned long getTimeSinceLastConnection() const;
	
	/// Force immediate reconnection attempt
	/// We use this when we detect connection issues
	void forceReconnect();
	
	/// Get number of connection attempts since startup
	[[nodiscard]] unsigned long getConnectionAttempts() const;

private:
	const char* ssid;
	const char* password;
	
	WiFiStatus currentStatus;
	unsigned long lastConnectionAttempt;
	unsigned long lastSuccessfulConnection;
	unsigned long connectionTimeout;
	unsigned long reconnectInterval;
	unsigned long connectionAttempts;
	
	/// Check if enough time has passed for reconnection attempt
	/// We implement exponential backoff to avoid overwhelming the network
	[[nodiscard]] bool shouldAttemptReconnect() const;
	
	/// Update internal status based on WiFi state
	/// We translate WiFi library status to our internal enum
	void updateStatus();
};

#endif /// WIFIMANAGER_H