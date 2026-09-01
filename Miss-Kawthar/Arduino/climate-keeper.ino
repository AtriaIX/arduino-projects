// gn pookies :3

#include "DHT.h"
#include <LiquidCrystal_I2C.h>

#define DHTPIN 3       
#define DHTTYPE DHT11

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

const int mq2Pin = A0;
const int flamePin = 2;
const int buzzer = 8;
const int redLed = 9;
const int blueLed = 10;

int smokeThreshold = 70;

void setup() {
  Serial.begin(9600);
  dht.begin();

  lcd.init();
  lcd.backlight();

  pinMode(mq2Pin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
}

void loop() {

  // sensor reading
  float humidity = dht.readHumidity();
  float temperatureC = dht.readTemperature();

  if (isnan(humidity)  isnan(temperatureC)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
    return;
  }
  // ===== LCD =====
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperatureC);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print(" %");
  
  // system
  int smokeValue = analogRead(mq2Pin);
  int flameValue = digitalRead(flamePin);

  // warning
  if (smokeValue > smokeThreshold  flameValue == LOW) {
    digitalWrite(redLed, HIGH);
    digitalWrite(blueLed, LOW);
    tone(buzzer, 1000);
  } else {
    digitalWrite(redLed, LOW);
    digitalWrite(blueLed, HIGH);
    noTone(buzzer);
  }

  Serial.print(humidity);
  Serial.print(",");
  Serial.print(temperatureC);
  Serial.print(",");
  Serial.print(smokeValue);
  Serial.print(",");
  Serial.println(flameValue);


  delay(2000);
}
