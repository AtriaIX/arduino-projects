/*
  Obstacle Avoiding Car
  ----------------------
  4WD robot car that drives forward until an ultrasonic sensor detects an
  obstacle within STOP_DISTANCE, then reverses, scans left/right with a
  servo-mounted sensor, and turns toward the clearer side.


  Required libraries (Sketch > Include Library > Add .ZIP File):
    - AFMotor : https://learn.adafruit.com/adafruit-motor-shield/library-install
    - NewPing : https://github.com/livetronic/Arduino-NewPing
    - Servo   : https://github.com/arduino-libraries/Servo.git
*/

#include <AFMotor.h>
#include <NewPing.h>
#include <Servo.h>

// ---------- Pin & hardware configuration ----------
#define TRIG_PIN      A0
#define ECHO_PIN      A1
#define SERVO_PIN     9

// ---------- Tunable parameters ----------
#define MAX_DISTANCE_CM   200   // max range the ultrasonic sensor will report
#define STOP_DISTANCE_CM  15    // obstacle distance that triggers an avoidance maneuver
#define NO_ECHO_CM        250   // distance to report when the sensor gets no echo (clear path)

#define MAX_SPEED         100   // top DC motor speed (0-255)
#define SPEED_RAMP_STEP   2     // how much to increase speed per ramp iteration
#define SPEED_RAMP_DELAY  5     // ms between ramp steps (avoids a big current draw spike)

#define SERVO_CENTER      115   // servo angle facing forward
#define SERVO_RIGHT       50
#define SERVO_LEFT        170
#define SERVO_MOVE_DELAY  500   // ms to let the servo reach its target angle

#define SCAN_SAMPLES      5     // number of ultrasonic readings taken per side when scanning
#define SCAN_SAMPLE_DELAY 30    // ms between individual scan readings

#define TURN_DURATION_MS  500   // how long to hold a turn before straightening out
#define LOOP_DELAY_MS     40    // small delay each main loop iteration

// ---------- Objects ----------
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE_CM);
Servo scanServo;

AF_DCMotor motors[4] = {
  AF_DCMotor(1, MOTOR12_1KHZ),
  AF_DCMotor(2, MOTOR12_1KHZ),
  AF_DCMotor(3, MOTOR34_1KHZ),
  AF_DCMotor(4, MOTOR34_1KHZ)
};

// ---------- State ----------
bool isMovingForward = false;
int  currentDistanceCm = 100;

// ===================================================================
//  Setup / main loop
// ===================================================================

void setup() {
  scanServo.attach(SERVO_PIN);
  scanServo.write(SERVO_CENTER);
  delay(2000); // let the servo settle before the first reading

  // Take a few throwaway readings so the sensor is "warmed up"
  for (int i = 0; i < 4; i++) {
    currentDistanceCm = readDistanceCm();
    delay(100);
  }
}

void loop() {
  delay(LOOP_DELAY_MS);

  if (currentDistanceCm <= STOP_DISTANCE_CM) {
    avoidObstacle();
  } else {
    moveForward();
  }

  currentDistanceCm = readDistanceCm();
}

// ===================================================================
//  Obstacle avoidance
// ===================================================================

void avoidObstacle() {
  moveStop();
  delay(100);

  moveBackward();
  delay(300);
  moveStop();
  delay(200);

  int distanceRight = scanDirection(SERVO_RIGHT);
  delay(200);
  int distanceLeft = scanDirection(SERVO_LEFT);
  delay(200);

  if (distanceRight >= distanceLeft) {
    turnRight();
  } else {
    turnLeft();
  }
  moveStop();
}

// Points the servo at a given angle, takes an averaged reading, then re-centers it.
int scanDirection(int servoAngle) {
  scanServo.write(servoAngle);
  delay(SERVO_MOVE_DELAY);

  int distance = readDistanceCmAveraged(SCAN_SAMPLES);
  delay(100);

  scanServo.write(SERVO_CENTER);
  return distance;
}

// Reads the ultrasonic sensor once; treats "no echo" as a clear/open path.
int readDistanceCm() {
  delay(70);
  int cm = sonar.ping_cm();
  return (cm == 0) ? NO_ECHO_CM : cm;
}

// Takes multiple readings and averages the valid ones for a steadier,
// more accurate distance value (a single ping is noisy/prone to misreads).
int readDistanceCmAveraged(int samples) {
  int total = 0;
  int validCount = 0;

  for (int i = 0; i < samples; i++) {
    int cm = sonar.ping_cm();
    if (cm != 0) { // 0 = no echo received, skip it rather than treating it as 0cm
      total += cm;
      validCount++;
    }
    delay(SCAN_SAMPLE_DELAY);
  }

  if (validCount == 0) {
    return NO_ECHO_CM; // no valid readings at all -> assume path is clear
  }
  return total / validCount;
}

// ===================================================================
//  Motor control
// ===================================================================

void moveStop() {
  for (int i = 0; i < 4; i++) {
    motors[i].run(RELEASE);
  }
}

void moveForward() {
  if (isMovingForward) return; // already going forward, nothing to do
  isMovingForward = true;

  for (int i = 0; i < 4; i++) {
    motors[i].run(FORWARD);
  }
  rampSpeed();
}

void moveBackward() {
  isMovingForward = false;

  for (int i = 0; i < 4; i++) {
    motors[i].run(BACKWARD);
  }
  rampSpeed();
}

// Gradually ramps all motors up to MAX_SPEED to avoid a sudden current draw.
void rampSpeed() {
  for (int speed = 0; speed < MAX_SPEED; speed += SPEED_RAMP_STEP) {
    for (int i = 0; i < 4; i++) {
      motors[i].setSpeed(speed);
    }
    delay(SPEED_RAMP_DELAY);
  }
}

void turnRight() {
  motors[0].run(FORWARD);
  motors[1].run(FORWARD);
  motors[2].run(BACKWARD);
  motors[3].run(BACKWARD);
  delay(TURN_DURATION_MS);

  for (int i = 0; i < 4; i++) {
    motors[i].run(FORWARD);
  }
}

void turnLeft() {
  motors[0].run(BACKWARD);
  motors[1].run(BACKWARD);
  motors[2].run(FORWARD);
  motors[3].run(FORWARD);
  delay(TURN_DURATION_MS);

  for (int i = 0; i < 4; i++) {
    motors[i].run(FORWARD);
  }
}
