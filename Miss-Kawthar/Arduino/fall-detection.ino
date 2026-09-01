#include <Wire.h>

const int MPU_ADDR = 0x68;

int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;

// Variables for calibration offsets
float gyroXOffset = 0, gyroYOffset = 0, gyroZOffset = 0;

float accPitch, accRoll;
// We now only need one set of variables for the final angles
float pitch = 0, roll = 0, yaw = 0; 

unsigned long prevTime;
float dt;

void setup() {
  Wire.begin();
  Serial.begin(115200); // Increased baud rate to prevent serial bottleneck
  pinMode(11, OUTPUT);
  // Wake up MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // --- Calibration Routine ---
  Serial.println("Calibrating Gyro... Please keep the sensor perfectly still.");
  long gyroXSum = 0, gyroYSum = 0, gyroZSum = 0;
  int numSamples = 500;
  
  for (int i = 0; i < numSamples; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43); // Start reading at gyro registers
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);
    
    gyroXSum += (Wire.read() << 8 | Wire.read());
    gyroYSum += (Wire.read() << 8 | Wire.read());
    gyroZSum += (Wire.read() << 8 | Wire.read());
    delay(3);
  }
  
  // Calculate average offsets
  gyroXOffset = (float)gyroXSum / numSamples;
  gyroYOffset = (float)gyroYSum / numSamples;
  gyroZOffset = (float)gyroZSum / numSamples;
  Serial.println("Calibration complete!");

  prevTime = micros(); // Use micros() for higher precision integration
}

void loop() {
  // Read raw data
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();

  Wire.read(); Wire.read(); // Skip temperature

  gyroX = Wire.read() << 8 | Wire.read();
  gyroY = Wire.read() << 8 | Wire.read();
  gyroZ = Wire.read() << 8 | Wire.read();

  // Time difference in seconds, using micros() for better precision
  unsigned long currentTime = micros();
  dt = (currentTime - prevTime) / 1000000.0; 
  prevTime = currentTime;

  // Accelerometer angles
  accPitch = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
  accRoll  = atan2(-accelX, accelZ) * 180.0 / PI;

  // Gyroscope rates (deg/sec) - Subtracting the calculated offsets!
  float gyroXrate = (gyroX - gyroXOffset) / 131.0;
  float gyroYrate = (gyroY - gyroYOffset) / 131.0;
  float gyroZrate = (gyroZ - gyroZOffset) / 131.0;

  // --- Corrected Complementary Filter ---
  // We integrate the gyro rate into the current filtered angle, then apply the accelerometer correction
  pitch = 0.96 * (pitch + gyroXrate * dt) + 0.04 * accPitch;
  roll  = 0.96 * (roll + gyroYrate * dt)  + 0.04 * accRoll;
  
  // Note: Yaw will still slowly drift over time because the MPU6050 lacks a magnetometer (compass) 
  // to correct it, but the calibration makes it drift much slower.
  yaw  += gyroZrate * dt; 

  // Output
  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print(" | Roll: ");
  Serial.print(roll);
  Serial.print(" | Yaw: ");
  Serial.println(yaw);
  if ((roll > 60) || (roll < -60))
  {
     digitalWrite(11,1);
  }
  else 
  {
    digitalWrite(11,0);
  }

  // Run at ~100Hz instead of 10Hz. Smooths out integration significantly.
  delay(10); 
}
