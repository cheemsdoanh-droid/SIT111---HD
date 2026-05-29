#include <Adafruit_LiquidCrystal.h>

// =====================================================
// SIT111 3.8HD - Tinkercad Test Version
// Smart Motion Detection Alarm System with Command Control
// Commands: ARM, DISARM, STATUS, TEST, RESET, HELP
// =====================================================

// Tinkercad I2C LCD
Adafruit_LiquidCrystal lcd(0);

// -------------------- Pin setup --------------------
const int PIR_PIN = 2;
const int GREEN_LED = 5;
const int RED_LED = 6;
const int RESET_BUTTON = 7;
const int BUZZER = 8;

// -------------------- Alarm states --------------------
enum AlarmState {
  DISARMED,
  ARMED,
  TRIGGERED
};

AlarmState currentState = DISARMED;

// -------------------- Variables --------------------
String inputCommand = "";
unsigned long lastBlinkTime = 0;
bool redBlinkState = false;

// =====================================================
// LCD helper functions
// =====================================================

void showLCD(String line1, String line2) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// =====================================================
// State helper functions
// =====================================================

String getStateName() {
  if (currentState == DISARMED) {
    return "DISARMED";
  } else if (currentState == ARMED) {
    return "ARMED";
  } else {
    return "TRIGGERED";
  }
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
    setArmed();
    Serial.println("Alarm reset. System returned to ARMED mode.");
  } else if (currentState == ARMED) {
    stopAlarmOutputs();
    digitalWrite(GREEN_LED, HIGH);
    showLCD("System Status:", "ARMED");
    Serial.println("System already armed. No triggered alarm to reset.");
  } else {
    stopAlarmOutputs();
    showLCD("System Status:", "DISARMED");
    Serial.println("System is disarmed. Reset completed.");
  }
}

void printStatus() {
  Serial.print("STATUS: ");
  Serial.println(getStateName());

  Serial.print("PIR reading: ");

  if (digitalRead(PIR_PIN) == HIGH) {
    Serial.println("MOTION DETECTED");
  } else {
    Serial.println("NO MOTION");
  }

  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("Available commands:");
  Serial.println("ARM     - Arm the motion alarm");
  Serial.println("DISARM  - Disarm the alarm and stop outputs");
  Serial.println("STATUS  - Show current alarm status");
  Serial.println("TEST    - Test LEDs and buzzer");
  Serial.println("RESET   - Reset triggered alarm back to armed mode");
  Serial.println("HELP    - Show this command list");
  Serial.println();
}

void testOutputs() {
  Serial.println("Running output test...");
  showLCD("TEST MODE", "LEDs + Buzzer");

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  tone(BUZZER, 800);
  delay(500);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 1200);
  delay(500);

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  tone(BUZZER, 1000);
  delay(500);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);

  Serial.println("Output test complete.");

  if (currentState == DISARMED) {
    setDisarmed();
  } else if (currentState == ARMED) {
    setArmed();
  } else {
    setTriggered();
  }
}

// =====================================================
// Serial command handling
// =====================================================

void handleCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command.length() == 0) {
    return;
  }

  Serial.print("Command received: ");
  Serial.println(command);

  if (command == "ARM") {
    setArmed();
  } else if (command == "DISARM") {
    setDisarmed();
  } else if (command == "STATUS") {
    printStatus();
  } else if (command == "TEST") {
    testOutputs();
  } else if (command == "RESET") {
    resetAlarm();
  } else if (command == "HELP") {
    printHelp();
  } else {
    Serial.print("Unknown command: ");
    Serial.println(command);
    Serial.println("Type HELP for available commands.");
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char incomingChar = Serial.read();

    if (incomingChar == '\n' || incomingChar == '\r') {
      handleCommand(inputCommand);
      inputCommand = "";
    } else {
      inputCommand += incomingChar;
    }

    // Tinkercad backup: process command once it matches a known command
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

// =====================================================
// Input checking
// =====================================================

void checkPIRSensor() {
  if (currentState == ARMED) {
    int motion = digitalRead(PIR_PIN);

    if (motion == HIGH) {
      setTriggered();
    }
  }
}

void checkResetButton() {
  // INPUT_PULLUP logic:
  // not pressed = HIGH
  // pressed = LOW
  if (digitalRead(RESET_BUTTON) == LOW) {
    delay(50);

    if (digitalRead(RESET_BUTTON) == LOW) {
      Serial.println("Physical reset button pressed.");
      resetAlarm();

      while (digitalRead(RESET_BUTTON) == LOW) {
        delay(10);
      }
    }
  }
}

void maintainTriggeredAlarm() {
  if (currentState == TRIGGERED) {
    tone(BUZZER, 1000);

    if (millis() - lastBlinkTime >= 300) {
      lastBlinkTime = millis();
      redBlinkState = !redBlinkState;
      digitalWrite(RED_LED, redBlinkState);
    }
  }
}

// =====================================================
// Arduino setup and loop
// =====================================================

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RESET_BUTTON, INPUT_PULLUP);

  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setBacklight(1);

  showLCD("Motion Alarm", "Starting...");
  delay(1500);

  setDisarmed();

  Serial.println();
  Serial.println("Tinkercad Smart Motion Detection Alarm System Ready.");
  Serial.println("Default state: DISARMED");
  printHelp();
}

void loop() {
  readSerialCommands();
  checkResetButton();
  checkPIRSensor();
  maintainTriggeredAlarm();
}