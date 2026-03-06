#include <Servo.h>

// ===== TB6612 pins =====
#define AIN1 7
#define AIN2 8
#define PWMA 5

#define BIN1 9
#define BIN2 10
#define PWMB 6

#define STBY 4

// ===== IR sensors =====
#define IR_RIGHT 2
#define IR_LEFT  3

// ===== MODE SWITCH =====
#define MODE_SW 11   // D11 to GND => obstacle mode

// ===== SERVO + ULTRASONIC =====
#define SERVO_PIN A1
#define TRIG_PIN  12
#define ECHO_PIN  13

// ===== STOP SIGNAL from Bin UNO =====
#define STOP_IN A2   // connect to Bin UNO D12 (wire), common GND

Servo scanServo;

// ===== SPEED SETTINGS =====
int baseSpeedA = 80;
int baseSpeedB = 80;
int turnSpeed  = 75;

// IR modules usually: BLACK = LOW
bool BLACK_IS_LOW = true;

// ===== Obstacle settings =====
int safeDistanceCM = 25;
int stopNearCM     = 10;
int rotateSpeed    = 110;

// ===== SERVO SETTINGS =====
int SERVO_CENTER = 90;
int SERVO_LEFT   = 35;
int SERVO_RIGHT  = 145;

// ----------------- Motor helpers -----------------
void motorLeftForward(int spd) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, constrain(spd, 0, 255));
}
void motorLeftBackward(int spd) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, constrain(spd, 0, 255));
}
void motorRightForward(int spd) {
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMB, constrain(spd, 0, 255));
}
void motorRightBackward(int spd) {
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, constrain(spd, 0, 255));
}

void forward(int spdA, int spdB) {
  motorLeftForward(spdA);
  motorRightForward(spdB);
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}

void rotateLeft(int spd) {
  motorLeftBackward(spd);
  motorRightForward(spd);
}
void rotateRight(int spd) {
  motorLeftForward(spd);
  motorRightBackward(spd);
}

// Line follow turns
void turnLeftSoft() {
  motorLeftForward(0);
  motorRightForward(turnSpeed);
}
void turnRightSoft() {
  motorLeftForward(turnSpeed);
  motorRightForward(0);
}

// ----------------- IR helper -----------------
bool isBlack(int raw) {
  if (BLACK_IS_LOW) return (raw == LOW);
  return (raw == HIGH);
}

// ----------------- Ultrasonic (stable) -----------------
long readDistanceOnceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  return duration / 58;
}

long readDistanceStableCM() {
  long a = readDistanceOnceCM(); delay(10);
  long b = readDistanceOnceCM(); delay(10);
  long c = readDistanceOnceCM();

  if (a > b) { long t=a; a=b; b=t; }
  if (b > c) { long t=b; b=c; c=t; }
  if (a > b) { long t=a; a=b; b=t; }
  return b;
}

// ----------------- Modes -----------------
void lineFollowMode() {
  bool leftBlack  = isBlack(digitalRead(IR_LEFT));
  bool rightBlack = isBlack(digitalRead(IR_RIGHT));

  if (leftBlack && rightBlack) {
    forward(baseSpeedA, baseSpeedB);
  } else if (leftBlack && !rightBlack) {
    turnLeftSoft();
  } else if (!leftBlack && rightBlack) {
    turnRightSoft();
  } else {
    stopMotors();
  }
}

void obstacleMode() {
  scanServo.write(SERVO_CENTER);
  delay(60);

  long front = readDistanceStableCM();

  if (front == 999) {
    forward(baseSpeedA, baseSpeedB);
    delay(120);
    stopMotors();
    return;
  }

  if (front <= stopNearCM) {
    stopMotors();
    delay(120);
    rotateRight(rotateSpeed);
    delay(250);
    stopMotors();
    return;
  }

  if (front > safeDistanceCM) {
    forward(baseSpeedA, baseSpeedB);
    return;
  }

  stopMotors();
  delay(120);

  scanServo.write(SERVO_LEFT);
  delay(180);
  long leftD = readDistanceStableCM();

  scanServo.write(SERVO_RIGHT);
  delay(180);
  long rightD = readDistanceStableCM();

  scanServo.write(SERVO_CENTER);
  delay(60);

  if (leftD > rightD) {
    rotateLeft(rotateSpeed);
    delay(260);
  } else {
    rotateRight(rotateSpeed);
    delay(260);
  }

  stopMotors();
  delay(60);

  forward(baseSpeedA, baseSpeedB);
  delay(220);
}

void setup() {
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);

  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  pinMode(IR_RIGHT, INPUT);
  pinMode(IR_LEFT, INPUT);

  pinMode(MODE_SW, INPUT_PULLUP);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // STOP input from Bin UNO
  pinMode(STOP_IN, INPUT_PULLUP); // STOP when LOW

  scanServo.attach(SERVO_PIN);
  scanServo.write(SERVO_CENTER);
  delay(300);

  stopMotors();
}

void loop() {
  // --- HARD OVERRIDE: if bin says STOP, stop immediately ---
  if (digitalRead(STOP_IN) == LOW) {
    stopMotors();
    delay(10);
    return;
  }

  bool obstacle = (digitalRead(MODE_SW) == LOW);

  if (obstacle) obstacleMode();
  else lineFollowMode();

  delay(10);
}

