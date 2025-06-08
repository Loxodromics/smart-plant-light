///
/// Main Test Program for Light Sensor
/// 
/// We test the VEML7700 light sensor by taking regular readings
/// and displaying both raw and averaged values. This helps us
/// validate sensor functionality and determine appropriate thresholds.
///

#include <Arduino.h>
#include <Wire.h>
#include "lightsensor.h"
#include "config.h"

LightSensor* lightSensor;

void setup() {
	/// We initialize serial communication for debugging output
	Serial.begin(115200);
	while (!Serial) {
		delay(10);
	}
	
	Serial.println("\n=== Smart Plant Light Controller - Light Sensor Test ===");
	
	/// We initialize I2C communication with custom pins
	Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
	Serial.print("I2C initialized - SDA: GPIO");
	Serial.print(I2C_SDA_PIN);
	Serial.print(", SCL: GPIO");
	Serial.println(I2C_SCL_PIN);
	
	/// We create and initialize the light sensor
	lightSensor = new LightSensor();
	
	if (lightSensor->begin()) {
		Serial.println("✓ Light sensor initialized successfully");
	} else {
		Serial.println("✗ Failed to initialize light sensor");
		Serial.println("Check I2C connections and sensor power");
		while (true) {
			delay(1000);
		}
	}
	
	Serial.print("Light threshold configured: ");
	Serial.print(LIGHT_THRESHOLD_LUX);
	Serial.println(" lux");
	Serial.print("Sensor samples for averaging: ");
	Serial.println(SENSOR_SAMPLES);
	Serial.println("\nStarting light measurements...");
	Serial.println("Try covering/uncovering the sensor to see changes");
	Serial.println();
}

void loop() {
	static unsigned long lastReadingTime = 0;
	static unsigned long lastDisplayTime = 0;
	const unsigned long readingInterval = 1000;  /// We read every second
	const unsigned long displayInterval = 2000;  /// We display every 2 seconds
	
	unsigned long currentTime = millis();
	
	/// We take sensor readings at regular intervals
	if (currentTime - lastReadingTime >= readingInterval) {
		lastReadingTime = currentTime;
		
		/// We attempt to read from the sensor
		bool success = lightSensor->updateReading();
		
		if (!success) {
			Serial.println("⚠ Sensor reading failed");
		}
	}
	
	/// We display results less frequently to avoid overwhelming the output
	if (currentTime - lastDisplayTime >= displayInterval) {
		lastDisplayTime = currentTime;
		
		/// We check sensor health first
		if (!lightSensor->isSensorHealthy()) {
			Serial.println("⚠ Sensor appears unhealthy - check connections");
			return;
		}
		
		/// We get current readings
		float rawLux = lightSensor->getLastRawLux();
		float avgLux = lightSensor->getCurrentLux();
		unsigned long readingCount = lightSensor->getReadingCount();
		bool belowThreshold = lightSensor->isBelowThreshold(LIGHT_THRESHOLD_LUX);
		
		/// We display the readings in a formatted way
		Serial.println("--- Light Sensor Reading ---");
		Serial.print("Raw reading: ");
		Serial.print(rawLux, 2);
		Serial.println(" lux");
		
		Serial.print("Averaged:    ");
		Serial.print(avgLux, 2);
		Serial.println(" lux");
		
		Serial.print("Threshold:   ");
		Serial.print(LIGHT_THRESHOLD_LUX, 1);
		Serial.println(" lux");
		
		Serial.print("Status:      ");
		if (belowThreshold) {
			Serial.println("🌙 DARK - Lights should turn ON");
		} else {
			Serial.println("☀️ BRIGHT - Lights should stay OFF");
		}
		
		Serial.print("Total readings: ");
		Serial.println(readingCount);
		
		/// We provide some context for the light levels
		Serial.print("Light level: ");
		if (avgLux < 1) {
			Serial.println("Very Dark");
		} else if (avgLux < 10) {
			Serial.println("Dark");
		} else if (avgLux < 100) {
			Serial.println("Dim Indoor");
		} else if (avgLux < 1000) {
			Serial.println("Normal Indoor");
		} else if (avgLux < 10000) {
			Serial.println("Bright Indoor");
		} else {
			Serial.println("Outdoor/Very Bright");
		}
		
		Serial.println();
	}
	
	/// We add a small delay to prevent overwhelming the system
	delay(50);
}

/// We clean up memory on program end
void cleanup() {
	if (lightSensor) {
		delete lightSensor;
		lightSensor = nullptr;
	}
}