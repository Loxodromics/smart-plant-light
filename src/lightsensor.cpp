///
/// LightSensor Implementation
/// 
/// We implement a robust light sensing system with averaging to smooth
/// out rapid fluctuations that could cause the plant lights to flicker
/// on and off inappropriately.
///

#include "lightsensor.h"
#include "config.h"

LightSensor::LightSensor() 
	: bufferSize(SENSOR_SAMPLES)
	, bufferIndex(0)
	, bufferFull(false)
	, currentAverageLux(0.0f)
	, lastRawLux(0.0f)
	, readingCount(0)
	, lastReadingTime(0)
	, sensorInitialized(false)
{
	/// We allocate memory for the averaging buffer
	/// Using dynamic allocation allows us to configure buffer size at compile time
	this->readingBuffer = new float[this->bufferSize];
	
	/// We initialize the buffer with zeros
	for (int i = 0; i < this->bufferSize; i++) {
		this->readingBuffer[i] = 0.0f;
	}
}

LightSensor::~LightSensor() {
	/// We clean up dynamically allocated memory
	delete[] this->readingBuffer;
}

bool LightSensor::begin() {
	/// We initialize I2C communication with the VEML7700
	if (!this->veml.begin()) {
		Serial.println("LightSensor: Failed to initialize VEML7700");
		return false;
	}
	
	/// We configure the sensor for optimal indoor lighting measurements
	/// ALS_GAIN_1 and ALS_100MS provide good balance of sensitivity and speed
	this->veml.setGain(VEML7700_GAIN_1);
	this->veml.setIntegrationTime(VEML7700_IT_100MS);
	
	/// We enable the ambient light sensor
	this->veml.enable(true);
	
	/// We wait for the sensor to stabilize after configuration
	delay(150);
	
	this->sensorInitialized = true;
	this->resetAveraging();
	
	Serial.println("LightSensor: VEML7700 initialized successfully");
	Serial.print("Buffer size for averaging: ");
	Serial.println(this->bufferSize);
	
	return true;
}

bool LightSensor::updateReading() {
	if (!this->sensorInitialized) {
		Serial.println("LightSensor: Sensor not initialized");
		return false;
	}
	
	/// We read the ambient light value in lux
	float newReading = this->veml.readLux();
	
	/// We validate the reading is reasonable
	/// VEML7700 returns NaN or very large values on error
	if (isnan(newReading) || newReading < 0 || newReading > 120000) {
		Serial.print("LightSensor: Invalid reading detected: ");
		Serial.println(newReading);
		return false;
	}
	
	/// We store the raw reading for diagnostics
	this->lastRawLux = newReading;
	this->lastReadingTime = millis();
	this->readingCount++;
	
	/// We add the new reading to our averaging buffer
	this->addToBuffer(newReading);
	this->calculateAverage();
	
	return true;
}

float LightSensor::getCurrentLux() const {
	return this->currentAverageLux;
}

float LightSensor::getLastRawLux() const {
	return this->lastRawLux;
}

bool LightSensor::isBelowThreshold(float thresholdLux) const {
	/// We use the averaged value for threshold comparison to avoid flickering
	return this->currentAverageLux < thresholdLux;
}

bool LightSensor::isSensorHealthy() const {
	if (!this->sensorInitialized) {
		return false;
	}
	
	/// We consider the sensor healthy if we've had recent successful readings
	/// and the readings are within expected ranges
	unsigned long timeSinceLastReading = millis() - this->lastReadingTime;
	bool recentReading = timeSinceLastReading < 60000; /// Within last minute
	bool validReading = !isnan(this->lastRawLux) && this->lastRawLux >= 0;
	
	return recentReading && validReading;
}

unsigned long LightSensor::getReadingCount() const {
	return this->readingCount;
}

void LightSensor::resetAveraging() {
	/// We clear the averaging buffer and reset state
	for (int i = 0; i < this->bufferSize; i++) {
		this->readingBuffer[i] = 0.0f;
	}
	
	this->bufferIndex = 0;
	this->bufferFull = false;
	this->currentAverageLux = 0.0f;
	
	Serial.println("LightSensor: Averaging buffer reset");
}

void LightSensor::calculateAverage() {
	float sum = 0.0f;
	int samplesCount = this->bufferFull ? this->bufferSize : this->bufferIndex;
	
	/// We sum all valid samples in the buffer
	for (int i = 0; i < samplesCount; i++) {
		sum += this->readingBuffer[i];
	}
	
	/// We calculate the average, avoiding division by zero
	if (samplesCount > 0) {
		this->currentAverageLux = sum / samplesCount;
	} else {
		this->currentAverageLux = 0.0f;
	}
}

void LightSensor::addToBuffer(float newReading) {
	/// We add the new reading to the circular buffer
	this->readingBuffer[this->bufferIndex] = newReading;
	
	/// We advance the buffer index
	this->bufferIndex++;
	
	/// We handle circular buffer wraparound
	if (this->bufferIndex >= this->bufferSize) {
		this->bufferIndex = 0;
		this->bufferFull = true;
	}
}