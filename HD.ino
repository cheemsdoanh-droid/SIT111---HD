#include "Waveshare_LCD1602.h"

Waveshare_LCD1602 lcd(16, 2);

// Pins
const int PIR_PIN = 2;
const int GREEN_LED = 5;
const int RED_LED = 6;
const int RESET_BUTTON = 7;
const int BUZZER = 8;

enum AlarmState { DISARMED, ARMED, TRIGGERED };
AlarmState currentState = DISARMED;

String inputCommand = "";
unsigned long lastBlinkTime = 0;
bool redBlinkState = false;

// PIR cooldown after reset
bool pirCooldownActive = false;
unsigned long pirCooldownStart = 0;
const unsigned long PIR_COOLDOWN_MS = 3000;

// Button: 5V -> D7, 10k to GND — press = HIGH
bool lastButtonReading = LOW;
bool buttonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 50;

void showLCD(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.send_string(line1.c_str());
  lcd.setCursor(0, 1);
  lcd.send_string(line2.c_str());
}

String getStateName() {
  if (currentState == DISARMED) return "DISARMED";
  if (currentState == ARMED) return "ARMED";
  return "TRIGGERED";
}

void stopAlarmOutputs() {
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);
  redBlinkState = false;
}

void setDisarmed() {
  currentState = DISARMED;
  digitalWrite(GREEN_LED, LOW);
  stopAlarmOutputs();
  showLCD("System Status:", "DISARMED");
  Serial.println("System disarmed. Motion is ignored.");
}

void setArmed() {
  currentState = ARMED;
  digitalWrite(GREEN_LED, HIGH);
  stopAlarmOutputs();
  showLCD("System Status:", "ARMED");
  Serial.println("System armed. Monitoring for motion...");
}

// Silent — skips serial print, used inside resetAlarm()
void setArmedSilent() {
  currentState = ARMED;
  digitalWrite(GREEN_LED, HIGH);
  stopAlarmOutputs();
  showLCD("System Status:", "ARMED");
}

void setTriggered() {
  currentState = TRIGGERED;
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 1000);
  showLCD("MOTION DETECTED", "Alarm Triggered");
  Serial.println("ALERT: Motion detected. Alarm triggered!");
}

void resetAlarm() {
  if (currentState == TRIGGERED) {
    Serial.println("Alarm reset. Returning to ARMED. PIR ignored for 3s.");
    setArmedSilent();
    pirCooldownActive = true;
    pirCooldownStart = millis();
  } else if (currentState == ARMED) {
    stopAlarmOutputs();
    digitalWrite(GREEN_LED, HIGH);
    showLCD("System Status:", "ARMED");
    Serial.println("Already armed. Nothing to reset.");
  } else {
    stopAlarmOutputs();
    showLCD("System Status:", "DISARMED");
    Serial.println("System disarmed. Reset done.");
  }
}

void printStatus() {
  Serial.print("STATUS: ");
  Serial.println(getStateName());
  Serial.print("PIR: ");
  Serial.println(digitalRead(PIR_PIN) == HIGH ? "MOTION" : "CLEAR");
  if (pirCooldownActive) {
    Serial.print("PIR cooldown: ");
    Serial.print((PIR_COOLDOWN_MS - (millis() - pirCooldownStart)) / 1000);
    Serial.println("s");
  }
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("Commands: ARM, DISARM, STATUS, TEST, RESET, HELP");
  Serial.println();
}

void testOutputs() {
  Serial.println("Running output test...");
  showLCD("TEST MODE", "LEDs + Buzzer");

  digitalWrite(GREEN_LED, HIGH); digitalWrite(RED_LED, LOW);
  tone(BUZZER, 800); delay(500);
  digitalWrite(GREEN_LED, LOW); digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 1200); delay(500);
  digitalWrite(GREEN_LED, HIGH); digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 1000); delay(500);
  digitalWrite(GREEN_LED, LOW); digitalWrite(RED_LED, LOW);
  noTone(BUZZER);

  Serial.println("Test complete.");
  if (currentState == DISARMED) setDisarmed();
  else if (currentState == ARMED) setArmed();
  else setTriggered();
}

void handleCommand(String command) {
  command.trim();
  command.toUpperCase();
  if (command.length() == 0) return;

  Serial.print("Command: ");
  Serial.println(command);

  if (command == "ARM") setArmed();
  else if (command == "DISARM") setDisarmed();
  else if (command == "STATUS") printStatus();
  else if (command == "TEST") testOutputs();
  else if (command == "RESET") resetAlarm();
  else if (command == "HELP") printHelp();
  else {
    Serial.print("Unknown: ");
    Serial.println(command);
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      handleCommand(inputCommand);
      inputCommand = "";
    } else {
      inputCommand += c;
    }

    // Tinkercad doesn't send newlines, so match as the string builds
    String temp = inputCommand;
    temp.trim();
    temp.toUpperCase();
    if (temp == "ARM" || temp == "DISARM" || temp == "STATUS" ||
        temp == "TEST" || temp == "RESET" || temp == "HELP") {
      handleCommand(temp);
      inputCommand = "";
    }
  }
}

void checkPIRSensor() {
  if (pirCooldownActive) {
    if (millis() - pirCooldownStart >= PIR_COOLDOWN_MS) {
      pirCooldownActive = false;
      Serial.println("PIR cooldown ended. Monitoring resumed.");
    } else {
      return;
    }
  }
  if (currentState == ARMED && digitalRead(PIR_PIN) == HIGH) {
    setTriggered();
  }
}

void checkResetButton() {
  bool reading = digitalRead(RESET_BUTTON);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }
  lastButtonReading = reading;

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (buttonState == LOW && reading == HIGH) {
      buttonState = HIGH;
      Serial.println("Reset button pressed.");
      resetAlarm();
    }
    if (buttonState == HIGH && reading == LOW) {
      buttonState = LOW;
    }
  }
}

void maintainTriggeredAlarm() {
  if (currentState != TRIGGERED) return;
  tone(BUZZER, 1000);
  if (millis() - lastBlinkTime >= 300) {
    lastBlinkTime = millis();
    redBlinkState = !redBlinkState;
    digitalWrite(RED_LED, redBlinkState);
  }
}

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RESET_BUTTON, INPUT);

  Serial.begin(9600);
  lcd.init();

  showLCD("Motion Alarm", "Starting...");
  delay(1500);
  setDisarmed();

  Serial.println("Alarm System Ready. Default: DISARMED");
  printHelp();
}

void loop() {
  readSerialCommands();
  checkResetButton();
  checkPIRSensor();
  maintainTriggeredAlarm();
}