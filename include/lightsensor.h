///
/// LightSensor - VEML7700 ambient light sensor interface
/// 
/// We implement smoothing and averaging to get stable light readings
/// despite potential noise or rapid changes in ambient conditions.
/// The sensor provides calibrated lux values which we can directly
/// compare against meaningful thresholds.
///

#ifndef LIGHTSENSOR_H
#define LIGHTSENSOR_H

#include <Arduino.h>
#include <Adafruit_VEML7700.h>

class LightSensor {
public:
	LightSensor();
	~LightSensor();
	
	/// Initialize the VEML7700 sensor with optimal settings
	/// We configure gain and integration time for indoor plant lighting scenarios
	[[nodiscard]] bool begin();
	
	/// Take a new sensor reading and update internal average
	/// Returns true if reading was successful, false if sensor error
	[[nodiscard]] bool updateReading();
	
	/// Get the current smoothed light level in lux
	/// We return the averaged value rather than raw readings for stability
	[[nodiscard]] float getCurrentLux() const;
	
	/// Get the most recent raw sensor reading without smoothing
	/// We provide this for diagnostics and calibration purposes
	[[nodiscard]] float getLastRawLux() const;
	
	/// Check if current light level is below the configured threshold
	/// We use this for the main plant light control decision
	[[nodiscard]] bool isBelowThreshold(float thresholdLux) const;
	
	/// Check if sensor is responding and providing valid data
	/// We use this to detect hardware failures or connection issues
	[[nodiscard]] bool isSensorHealthy() const;
	
	/// Get number of successful readings taken since initialization
	/// We track this to validate sensor reliability over time
	[[nodiscard]] unsigned long getReadingCount() const;
	
	/// Reset the averaging buffer and statistics
	/// We use this when we want to start fresh after a configuration change
	void resetAveraging();

private:
	Adafruit_VEML7700 veml;
	
	/// Circular buffer for averaging light readings
	float* readingBuffer;
	int bufferSize;
	int bufferIndex;
	bool bufferFull;
	
	/// Current state tracking
	float currentAverageLux;
	float lastRawLux;
	unsigned long readingCount;
	unsigned long lastReadingTime;
	bool sensorInitialized;
	
	/// Calculate the current average from the buffer
	/// We recalculate this each time to handle the circular buffer properly
	void calculateAverage();
	
	/// Add a new reading to the circular buffer
	/// We manage the buffer index and full state automatically
	void addToBuffer(float newReading);
};

#endif /// LIGHTSENSOR_H