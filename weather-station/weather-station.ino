// Определяем пины
const int rainDigitalPin = 2; // D0 датчика
const int rainAnalogPin = A0; // A0 датчика

void setup() {
  pinMode(rainDigitalPin, INPUT);
  Serial.begin(9600);
  Serial.println("Система мониторинга дождя запущена...");
}

void loop() {
  // 1. Читаем цифровое значение (0 или 1)
  int isRainingDigital = digitalRead(rainDigitalPin);

  // 2. Читаем аналоговое значение (0 - 1023)
  int rainIntensity = analogRead(rainAnalogPin);

  // Вывод в монитор порта
  Serial.print("Цифровой порог: ");
  if (isRainingDigital == LOW) {
    Serial.print("ОБНАРУЖЕН ");
  } else {
    Serial.print("НЕТ ");
  }

  Serial.print("| Аналоговое значение: ");
  Serial.println(rainIntensity);

  // Логика на основе аналоговых данных
  if (rainIntensity < 300) {
    Serial.println("Статус: Сильный ливень!");
  } else if (rainIntensity < 700) {
    Serial.println("Статус: Идет небольшой дождь.");
  } else {
    Serial.println("Статус: Сухо.");
  }

  delay(1000); // Задержка в 1 секунду
}