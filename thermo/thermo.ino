#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"


#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Настройки дисплея (адрес 0x27, 16 символов, 2 строки)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Инициализация датчика и дисплея
  dht.begin();
  
  lcd.init();
  lcd.backlight();
  
  // Стартовое сообщение
  lcd.setCursor(0, 0);
  lcd.print("Misha's Station");
  lcd.setCursor(0, 1);
  lcd.print("Loading...");
  delay(2000);
  lcd.clear();
}

void loop() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Проверка на ошибки
  if (isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!  ");
  } else {
    // Вывод температуры
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print((int)t); // Целое число для экономии места
    lcd.print(" C");
    lcd.print("      "); // Очистка хвоста строки

    // Вывод влажности
    lcd.setCursor(0, 1);
    lcd.print("Hum:  ");
    lcd.print((int)h);
    lcd.print(" %");
    lcd.print("      ");
  }

  // Обновляем раз в 2 секунды (DHT11 не любит чаще)
  delay(2000);
}