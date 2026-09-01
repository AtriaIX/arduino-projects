# 🏥 Hospital Monitoring System

A simple Arduino-based patient monitoring system. It covers environmental safety, biometric sensing, medication management, and fall detection .. with all data reported to Google Sheets via Wi-Fi.

---

## Modules

**Heart Rate Monitor**  
An Arduino reads data from a Pulse sensor, a homemade GSR sensor, and an MPU6050 at 100Hz, and sends it over serial to MATLAB. MATLAB handles all signal processing .. bandpass/lowpass filtering, peak detection, and BPM calculation .. and classifies the patient's state (Resting, Active, Stressed, or Panic Attack) in a live UI.

**Time Medicine Reminder & Glucose Detection**  
An RTC keeps track of time and triggers servo motors to open medicine box compartments at scheduled intervals. An LCD shows the current time and next dose, and a buzzer + button confirm the patient took their medicine. A water level sensor on the same board monitors glucose levels (water level) and buzzes on low readings.

**Safe Climate Keeper**  
Monitors room temperature and humidity using a DHT sensor (displayed on LCD). An MQ2 gas sensor and IR flame sensor watch for hazards .. a buzzer triggers when any smoke or flame are detected.

**Fall Detection & Data Storage**  
An MPU6050 tracks patient orientation and triggers a buzzer if a fall is detected (roll > ±60°). An ESP8266 Wi-Fi module on the same board collects data from all modules and sends periodic reports to Google Sheets via the Google Sheets API.

> The project can be built with only 2 Arduinos btw, one for the Heart Rate Monitor (required for MATLAB serial at 100Hz), and one handling all other modules.\
> and at the end .. we all know that miss kawthar do all that better than any smart system exists :3

---
