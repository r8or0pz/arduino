#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int rainDigitalPin = 2;
const int rainAnalogPin = A0;

unsigned long lastReport = 0;
const unsigned long reportInterval = 1000;

// LCD configuration: address 0x27, 20 chars, 4 lines
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  pinMode(rainDigitalPin, INPUT_PULLUP);
  Serial.begin(115200); // Matched to your deploy.sh

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Weather Station");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  Serial.println("--- Weather Station: Rain Sensor Active ---");
}

void loop() {
  // Non-blocking timer
  if (millis() - lastReport >= reportInterval) {
    lastReport = millis();

    int isRainingDigital = digitalRead(rainDigitalPin);
    int rainIntensity = analogRead(rainAnalogPin);

    // Serial Report
    Serial.print("Digital: ");
    Serial.print(isRainingDigital == LOW ? "ACTIVE" : "IDLE");
    Serial.print(" | Raw Intensity: ");
    Serial.println(rainIntensity);

    // LCD Report
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RAIN SENSOR");

    lcd.setCursor(0, 1);
    lcd.print("Status: ");
    lcd.print(isRainingDigital == LOW ? "Raining" : "Dry");

    lcd.setCursor(0, 2);
    lcd.print("Intensity: ");
    lcd.print(rainIntensity);

    lcd.setCursor(0, 3);
    // Classification
    if (rainIntensity < 300) {
      lcd.print("Heavy Rain");
      Serial.println("Status: Heavy Rain");
    } else if (rainIntensity < 700) {
      lcd.print("Light Rain");
      Serial.println("Status: Light Rain");
    } else {
      lcd.print("No Rain");
      Serial.println("Status: Dry");
    }
  }
}
