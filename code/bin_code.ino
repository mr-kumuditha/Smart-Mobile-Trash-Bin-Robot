#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// --------- I2C LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== STOP SIGNAL to Robot UNO =====
// Wire: Bin UNO D12  -> Robot UNO A2
// Also connect GND <-> GND between both UNOs
#define STOP_OUT 12   // LOW = STOP robot, HIGH = RUN robot

// --------- Full ultrasonic (inside bin) ----------
#define FULL_TRIG 2
#define FULL_ECHO 3

// --------- Hand ultrasonic ----------
#define HAND_TRIG 4
#define HAND_ECHO 5

// --------- Servo (lid) ----------
#define SERVO_PIN 11

// --------- LEDs + buzzer ----------
#define LED_GREEN 7
#define LED_BLUE  8
#define BUZZER    9
#define LED_RED   10

Servo lid;

// --------- Servo angles ----------
const int SERVO_CLOSED = 0;
const int SERVO_OPEN   = 90;

// Smooth motion settings
const int SERVO_STEP_DEG = 1;          // 1 degree per step
const int SERVO_STEP_DELAY_MS = 8;     // higher = slower/smoother
int currentServoAngle = SERVO_CLOSED;

// --------- Thresholds ----------
const int HAND_OPEN_CM = 15;
const int FULL_CM      = 8;

// Timers
const unsigned long OPEN_TIME_MS = 5000;         // lid stays open
const unsigned long CLOSE_COUNTDOWN_MS = 3000;   // show closing countdown

// --------- Backlight (Option A: OFF normally, ON when needed) ----------
const unsigned long BACKLIGHT_IDLE_OFF_MS = 2000;
const unsigned long BACKLIGHT_ON_AFTER_HAND_MS = 8000;

unsigned long backlightUntil = 0;
bool backlightState = true;

// --------- Idle message rotation ----------
const unsigned long IDLE_ROTATE_MS = 2000;
unsigned long lastIdleRotate = 0;
int idleScreen = 0;

// Keep last measured distances for display
long lastFullDist = 999;
long lastHandDist = 999;

// ---------- Ultrasonic helpers ----------
long readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 999;
  return duration / 58;
}

long readStableCM(int trigPin, int echoPin) {
  long a = readDistanceCM(trigPin, echoPin); delay(10);
  long b = readDistanceCM(trigPin, echoPin); delay(10);
  long c = readDistanceCM(trigPin, echoPin);

  // median of 3
  if (a > b) { long t=a; a=b; b=t; }
  if (b > c) { long t=b; b=c; c=t; }
  if (a > b) { long t=a; a=b; b=t; }
  return b;
}

// ---------- LCD helpers ----------
void lcdLine(int row, const char* msg) {
  lcd.setCursor(0, row);
  lcd.print(msg);
  int len = 0;
  while (msg[len] != '\0') len++;
  for (int i = len; i < 16; i++) lcd.print(' ');
}

void setBacklight(bool on) {
  if (on && !backlightState) {
    lcd.backlight();
    backlightState = true;
  } else if (!on && backlightState) {
    lcd.noBacklight();
    backlightState = false;
  }
}

void showDistances(long handCm, long fullCm) {
  lcd.setCursor(0, 0);
  lcd.print("HAND:");
  if (handCm == 999) lcd.print("ERR ");
  else { lcd.print(handCm); lcd.print("CM "); }
  lcd.print("      ");

  lcd.setCursor(0, 1);
  lcd.print("FULL:");
  if (fullCm == 999) lcd.print("ERR ");
  else { lcd.print(fullCm); lcd.print("CM "); }
  lcd.print("      ");
}

// ---------- Servo smooth movement ----------
void servoWriteTracked(int angle) {
  angle = constrain(angle, 0, 180);
  lid.write(angle);
  currentServoAngle = angle;
}

void servoSmoothTo(int targetAngle) {
  targetAngle = constrain(targetAngle, 0, 180);
  if (targetAngle == currentServoAngle) return;

  int step = (targetAngle > currentServoAngle) ? SERVO_STEP_DEG : -SERVO_STEP_DEG;

  for (int a = currentServoAngle; a != targetAngle; a += step) {
    lid.write(a);
    delay(SERVO_STEP_DELAY_MS);
  }
  lid.write(targetAngle);
  currentServoAngle = targetAngle;
}

void lidCloseSafe() {
  digitalWrite(LED_BLUE, LOW);
  noTone(BUZZER);
  servoWriteTracked(SERVO_CLOSED);
}

// ---------- Setup ----------
void setup() {
  pinMode(FULL_TRIG, OUTPUT);
  pinMode(FULL_ECHO, INPUT);

  pinMode(HAND_TRIG, OUTPUT);
  pinMode(HAND_ECHO, INPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // STOP output to Robot UNO
  pinMode(STOP_OUT, OUTPUT);
  digitalWrite(STOP_OUT, HIGH); // allow robot to run by default

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  noTone(BUZZER);

  lid.attach(SERVO_PIN);
  servoWriteTracked(SERVO_CLOSED);

  lcd.init();
  lcd.backlight();
  backlightState = true;

  lcdLine(0, "SMART BIN");
  lcdLine(1, "BOOTING...");
  delay(900);

  backlightUntil = millis() + BACKLIGHT_IDLE_OFF_MS;
  lastIdleRotate = millis();
  idleScreen = 0;
}

// ---------- Loop ----------
void loop() {
  // Backlight auto control
  if (millis() < backlightUntil) setBacklight(true);
  else setBacklight(false);

  // Read sensors
  lastFullDist = readStableCM(FULL_TRIG, FULL_ECHO);
  lastHandDist = readStableCM(HAND_TRIG, HAND_ECHO);

  bool fullSensorOk = (lastFullDist != 999);
  bool handSensorOk = (lastHandDist != 999);

  // If both sensors failing -> stop robot for safety
  if (!fullSensorOk && !handSensorOk) {
    digitalWrite(STOP_OUT, LOW);           // STOP robot
    backlightUntil = millis() + 6000;

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    lidCloseSafe();

    lcdLine(0, "SENSOR ERROR");
    lcdLine(1, "CHECK WIRE");
    tone(BUZZER, 2000); delay(200);
    noTone(BUZZER);

    delay(200);
    return;
  }

  // FULL check
  bool binFull = (fullSensorOk && lastFullDist <= FULL_CM);

  if (binFull) {
    digitalWrite(STOP_OUT, LOW);           // STOP robot when bin full
    backlightUntil = millis() + 6000;

    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    lidCloseSafe();

    lcdLine(0, "BIN FULL !!!");
    lcdLine(1, "EMPTY NEEDED");

    // Strong buzzer pattern
    tone(BUZZER, 2500); delay(250);
    noTone(BUZZER);     delay(100);
    tone(BUZZER, 2500); delay(250);
    noTone(BUZZER);

    delay(150);
    return;
  } else {
    // Normal running allowed (unless hand action triggers)
    digitalWrite(STOP_OUT, HIGH);          // RUN robot
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }

  // Idle rotating messages
  if (millis() - lastIdleRotate >= IDLE_ROTATE_MS) {
    lastIdleRotate = millis();
    idleScreen++;
    if (idleScreen > 3) idleScreen = 0;
  }

  if (idleScreen == 0) {
    lcdLine(0, "BIN OK");
    lcdLine(1, "SHOW HAND");
  } else if (idleScreen == 1) {
    lcdLine(0, "READY");
    lcdLine(1, "PUT WASTE IN");
  } else if (idleScreen == 2) {
    lcdLine(0, "KEEP CAMPUS");
    lcdLine(1, "CLEAN  :)");
  } else {
    showDistances(lastHandDist, lastFullDist);
  }

  // Hand detect => STOP robot, open lid, then allow robot again
  if (handSensorOk && lastHandDist <= HAND_OPEN_CM) {
    digitalWrite(STOP_OUT, LOW);           // STOP robot immediately
    backlightUntil = millis() + BACKLIGHT_ON_AFTER_HAND_MS;
    setBacklight(true);

    lcdLine(0, "HAND DETECTED");
    lcdLine(1, "OPENING...");
    delay(200);

    // Smooth open
    digitalWrite(LED_BLUE, HIGH);
    servoSmoothTo(SERVO_OPEN);

    unsigned long start = millis();
    int lastShown = -1;

    // Open countdown with beep tone pattern
    while (millis() - start < OPEN_TIME_MS) {
      unsigned long elapsed = millis() - start;
      int remaining = (int)((OPEN_TIME_MS - elapsed + 999) / 1000);
      if (remaining < 0) remaining = 0;

      if (remaining != lastShown) {
        lastShown = remaining;
        lcdLine(0, "LID OPEN");
        lcd.setCursor(0, 1);
        lcd.print("CLOSE IN: ");
        lcd.print(remaining);
        lcd.print("S     ");
      }

      if ((millis() / 200) % 2 == 0) tone(BUZZER, 2200);
      else noTone(BUZZER);

      delay(10);
    }

    noTone(BUZZER);

    // Closing countdown
    lcdLine(0, "LID CLOSING");
    unsigned long cStart = millis();
    int lastClose = -1;

    while (millis() - cStart < CLOSE_COUNTDOWN_MS) {
      unsigned long e = millis() - cStart;
      int remainClose = (int)((CLOSE_COUNTDOWN_MS - e + 999) / 1000);
      if (remainClose < 0) remainClose = 0;

      if (remainClose != lastClose) {
        lastClose = remainClose;
        lcd.setCursor(0, 1);
        lcd.print("CLOSING: ");
        lcd.print(remainClose);
        lcd.print("S      ");
      }

      if (remainClose <= 1) tone(BUZZER, 2600);
      else noTone(BUZZER);

      delay(10);
    }

    noTone(BUZZER);

    // Smooth close
    servoSmoothTo(SERVO_CLOSED);
    digitalWrite(LED_BLUE, LOW);

    lcdLine(0, "THANK YOU!");
    lcdLine(1, "KEEP CLEAN :)");
    delay(900);

    backlightUntil = millis() + 2000;

    // Allow robot to move again after lid closes
    digitalWrite(STOP_OUT, HIGH);          // RUN robot

    // Prevent immediate re-trigger
    delay(800);
  }

  delay(80);
}

