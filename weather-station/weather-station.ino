#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Sensors.h>
#include <Logger.h>

const int rainDigitalPin = 2;
const int rainAnalogPin = A0;
const int touchPin = 7;

unsigned long lastReport = 0;
const unsigned long reportInterval = 1000;

bool isDisplayOn = true;

// LCD configuration: address 0x27, 20 chars, 4 lines
LiquidCrystal_I2C lcd(0x27, 20, 4);
RainSensor rainSensor("RainDigital", rainDigitalPin, "RainAnalog", rainAnalogPin);
TouchSensor touchButton("DisplayTouch", touchPin, 120, false, false);

void setup() {
  Logger::begin(115200);
  rainSensor.begin();
  touchButton.begin();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Weather Station");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  Logger::info("weather-station", "Rain sensors initialized");
}

void loop() {
  if (touchButton.wasPressed()) {
    isDisplayOn = !isDisplayOn;
    if (isDisplayOn) {
      lcd.backlight();
      Logger::info("weather-station", "Display: ON");
    } else {
      lcd.noBacklight();
      Logger::info("weather-station", "Display: OFF");
    }
  }

  // Non-blocking timer
  if (millis() - lastReport >= reportInterval) {
    lastReport = millis();

    RainReading rain = rainSensor.read();

    // Serial Report
    String logLine = "Digital: ";
    logLine += (rain.isRaining ? "ACTIVE" : "IDLE");
    logLine += " | Raw Intensity: ";
    logLine += rain.intensity;
    Logger::info("weather-station", logLine.c_str());

    // LCD Report
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RAIN SENSOR");

    lcd.setCursor(0, 1);
    lcd.print("Status: ");
    lcd.print(rain.isRaining ? "Raining" : "Dry");

    lcd.setCursor(0, 2);
    lcd.print("Intensity: ");
    lcd.print(rain.intensity);

    lcd.setCursor(0, 3);
    lcd.print(RainSensor::levelToText(rain.level));

    Logger::log(rain.level == RAIN_HEAVY ? WARN : INFO, "weather-station", RainSensor::levelToStatusText(rain.level));
  }
}
