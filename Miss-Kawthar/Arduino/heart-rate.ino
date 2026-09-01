#include <Wire.h>

const int MPU_ADDR = 0x68; 
int16_t AcX, AcY, AcZ;
int buzzerPin = 8;
void setup() {
  Serial.begin(115200); 
pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  // Wake up the MPU6050 motion sensor
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);
}

void loop() {
  // READ MOTION 
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true); 
  
  AcX = Wire.read()<<8 | Wire.read(); // X-axis
  AcY = Wire.read()<<8 | Wire.read(); // Y-axis
  AcZ = Wire.read()<<8 | Wire.read(); // Z-axis
  
 
  float ax = AcX / 16384.0;
  float ay = AcY / 16384.0;
  float az = AcZ / 16384.0;
  
  float motionMag = abs(sqrt(ax*ax + ay*ay + az*az) - 1.0);

  // preparing the pulse sensor
 const int N = 30;
static int buffer[N];
static int index = 0;
static long sum = 0;

sum -= buffer[index];
buffer[index] = analogRead(A1);
sum += buffer[index];
index = (index + 1) % N;

int pulseValue = sum / N;

  int sweatValue = analogRead(A0);

  // sending data to matlab
  Serial.print(pulseValue);
  Serial.print(",");
  Serial.print(motionMag);
  Serial.print(",");
  Serial.println(sweatValue);
  
  if (Serial.available() > 0) {
    char command = Serial.read(); 
    
    if (command == '1') {
      digitalWrite(buzzerPin, HIGH); 
    } 
    else if (command == '0') {
      digitalWrite(buzzerPin, LOW);  
    }
  }
  delay(10); 
}
