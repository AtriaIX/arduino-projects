#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== Pins =====
int buzzer = 9;
int button = 10;
int levelsen = A0;
int watbuz = 8;

// ===== Servo =====
Servo servo1;
Servo servo2;
Servo servo3;

// ===== Variables =====
int senval = 0;
int thresholdval = 100;

// medicine time
int medHours[3];
int medMinutes[3];

int totalMeds = 3;
int currentMed = 0;

bool alarmActive = false;

// ===== Setup =====
void setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(watbuz, OUTPUT);

  // Servo attach
  servo1.attach(3);
  servo2.attach(5);
  servo3.attach(6);

  // starting closed
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);

  lcd.init();
  lcd.backlight();

  if (!rtc.begin()) {
    lcd.print("RTC ERROR");
    while (1);
  }
  DateTime now = rtc.now();

for (int i = 0; i < 3; i++) {
  int newMinute = now.minute() + (i + 1);
  int newHour = now.hour();

  // more than 60 mins
  if (newMinute >= 60) {
    newMinute -= 60;
    newHour += 1;
  }

  // if more than 24 hours
  if (newHour >= 24) {
    newHour = 0;
  }

  medHours[i] = newHour;
  medMinutes[i] = newMinute;
}
}

// ===== Loop =====
void loop() {

  levelSensor();

  // button -> closes the buzzer and returns the servo
  if (digitalRead(button) == LOW) {
    noTone(buzzer);
    alarmActive = false;

    servo1.write(0);
    servo2.write(0);
    servo3.write(0);

    // next medicine time
    currentMed++;
    if (currentMed >= totalMeds) {
      currentMed = 0;
    }

    lcd.clear();
    delay(200);
  }

  DateTime now = rtc.now();

  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  // displaying time
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  print2digits(hour);
  lcd.print(":");
  print2digits(minute);
  lcd.print(":");
  print2digits(second);

  // displaying next medicine time
  lcd.setCursor(0, 1);
  lcd.print("Next: ");
  print2digits(medHours[currentMed]);
  lcd.print(":");
  print2digits(medMinutes[currentMed]);

  // if its medicine time
  if (hour == medHours[currentMed] && minute == medMinutes[currentMed] && !alarmActive) {
    alarmActive = true;
    lcd.clear();

    moveServo(currentMed); 
  }

  // notifying
  if (alarmActive) {

    lcd.setCursor(0, 0);
    lcd.print("Take Medicine!");
    lcd.setCursor(0, 1);
    lcd.print("Press Button");

    playMedicineTone();
  }

  delay(100);
}

// ===== Level Sensor =====
void levelSensor() {
  senval = analogRead(levelsen);

  if (senval < thresholdval) {
    digitalWrite(watbuz, HIGH);
    delay(100);
    digitalWrite(watbuz, LOW);
  } else {
    digitalWrite(watbuz, LOW);
  }
}

// ===== Print 2 digits =====
void print2digits(int number) {
  if (number < 10) lcd.print("0");
  lcd.print(number);
}

// ===== Move Servo =====
void moveServo(int medIndex) {
  if (medIndex == 0) {
    servo1.write(90);
  }
  else if (medIndex == 1) {
    servo2.write(90);
  }
  else if (medIndex == 2) {
    servo3.write(90);
  }
}

// ===== Buzzer Tone =====
void playMedicineTone() {

  if (!alarmActive) return;

  tone(buzzer, 800);
  delay(120);
  if (digitalRead(button) == LOW) { noTone(buzzer); return; }

  tone(buzzer, 1000);
  delay(120);
  if (digitalRead(button) == LOW) { noTone(buzzer); return; }

  tone(buzzer, 1200);
  delay(120);
  noTone(buzzer);
  delay(150);

  tone(buzzer, 1200);
  delay(120);
  if (digitalRead(button) == LOW) { noTone(buzzer); return; }

  tone(buzzer, 1000);
  delay(120);
  tone(buzzer, 800);
  delay(120);

  noTone(buzzer);
}
