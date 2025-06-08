///
/// WiFiManager Implementation
/// 
/// We implement a robust WiFi connection system that handles
/// network outages and reconnection automatically. This ensures
/// our plant light controller stays connected for time sync.
///

#include "wifimanager.h"
#include "config.h"

WiFiManager::WiFiManager(const char* ssid, const char* password)
	: ssid(ssid)
	, password(password)
	, currentStatus(WiFiStatus::Disconnected)
	, lastConnectionAttempt(0)
	, lastSuccessfulConnection(0)
	, connectionTimeout(WIFI_TIMEOUT_MS)
	, reconnectInterval(30000) /// We start with 30 second intervals
	, connectionAttempts(0)
{
	/// We initialize member variables in constructor for clean state
}

void WiFiManager::begin() {
	/// We set WiFi mode to station (client) mode
	WiFi.mode(WIFI_STA);
	
	/// We set a reasonable hostname for network identification
	WiFi.setHostname("PlantLightController");
	
	/// We disable auto-reconnect to handle it ourselves
	WiFi.setAutoReconnect(false);
	
	Serial.println("WiFiManager: Initialized");
	Serial.print("Target network: ");
	Serial.println(this->ssid);
	
	/// We attempt initial connection
	this->connect();
}

void WiFiManager::update() {
	this->updateStatus();
	
	/// We check if we need to attempt reconnection
	if (this->currentStatus != WiFiStatus::Connected && this->shouldAttemptReconnect()) {
		Serial.println("WiFiManager: Attempting reconnection...");
		this->connect();
	}
}

bool WiFiManager::connect() {
	this->lastConnectionAttempt = millis();
	this->connectionAttempts++;
	this->currentStatus = WiFiStatus::Connecting;
	
	Serial.print("WiFiManager: Connecting to ");
	Serial.print(this->ssid);
	Serial.print(" (attempt #");
	Serial.print(this->connectionAttempts);
	Serial.println(")");
	
	/// We start the connection process
	WiFi.begin(this->ssid, this->password);
	
	/// We wait for connection with timeout
	unsigned long startTime = millis();
	while (WiFi.status() != WL_CONNECTED && 
		millis() - startTime < this->connectionTimeout) {
		delay(250);
		Serial.print(".");
	}
	Serial.println();
	
	/// We check if connection was successful
	if (WiFi.status() == WL_CONNECTED) {
		this->currentStatus = WiFiStatus::Connected;
		this->lastSuccessfulConnection = millis();
		this->reconnectInterval = 30000; /// We reset to base interval on success
		
		Serial.println("WiFiManager: ✓ Connected successfully");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		Serial.print("Signal strength: ");
		Serial.print(WiFi.RSSI());
		Serial.println(" dBm");
		
		return true;
	} else {
		this->currentStatus = WiFiStatus::Failed;
		
		/// We implement exponential backoff for failed connections
		/// This prevents overwhelming the network with rapid retry attempts
		this->reconnectInterval = min(this->reconnectInterval * 2, 300000UL); /// Cap at 5 minutes
		
		Serial.println("WiFiManager: ✗ Connection failed");
		Serial.print("Next attempt in ");
		Serial.print(this->reconnectInterval / 1000);
		Serial.println(" seconds");
		
		return false;
	}
}

WiFiStatus WiFiManager::getStatus() const {
	return this->currentStatus;
}

bool WiFiManager::isConnected() const {
	return this->currentStatus == WiFiStatus::Connected && WiFi.status() == WL_CONNECTED;
}

int WiFiManager::getSignalStrength() const {
	if (this->isConnected()) {
		return WiFi.RSSI();
	}
	return -999; /// We return invalid value when not connected
}

String WiFiManager::getLocalIP() const {
	if (this->isConnected()) {
		return WiFi.localIP().toString();
	}
	return "Not Connected";
}

unsigned long WiFiManager::getTimeSinceLastConnection() const {
	if (this->lastSuccessfulConnection == 0) {
		return ULONG_MAX; /// We return max value if never connected
	}
	return millis() - this->lastSuccessfulConnection;
}

void WiFiManager::forceReconnect() {
	Serial.println("WiFiManager: Forcing reconnection...");
	WiFi.disconnect();
	delay(100);
	this->currentStatus = WiFiStatus::Disconnected;
	this->connect();
}

unsigned long WiFiManager::getConnectionAttempts() const {
	return this->connectionAttempts;
}

bool WiFiManager::shouldAttemptReconnect() const {
	/// We don't reconnect if we're currently connecting
	if (this->currentStatus == WiFiStatus::Connecting) {
		return false;
	}
	
	/// We check if enough time has passed since last attempt
	return millis() - this->lastConnectionAttempt >= this->reconnectInterval;
}

void WiFiManager::updateStatus() {
	/// We update our internal status based on actual WiFi state
	wl_status_t wifiStatus = WiFi.status();
	
	switch (wifiStatus) {
		case WL_CONNECTED:
			if (this->currentStatus != WiFiStatus::Connected) {
				/// We just connected - update our records
				this->currentStatus = WiFiStatus::Connected;
				this->lastSuccessfulConnection = millis();
			}
			break;
			
		case WL_CONNECT_FAILED:
		case WL_CONNECTION_LOST:
		case WL_DISCONNECTED:
			if (this->currentStatus == WiFiStatus::Connected) {
				Serial.println("WiFiManager: Connection lost");
				this->currentStatus = WiFiStatus::Disconnected;
			}
			break;
			
		default:
			/// We handle other states (scanning, etc.) as disconnected
			if (this->currentStatus == WiFiStatus::Connected) {
				this->currentStatus = WiFiStatus::Disconnected;
			}
			break;
	}
}