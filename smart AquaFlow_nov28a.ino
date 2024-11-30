#include <Servo.h>
#include <EEPROM.h>

// Pin definitions
const int trigPin = 9;
const int echoPin = 10;
const int buzzerPin = 8;
const int servoPin = 7;

// Distance thresholds (in cm)
const int activateMinDistance = 45;
const int activateMaxDistance = 60;
const int stopDistance = 10;

// State variables
bool buzzerOn = false;

// Time and distance tracking variables
unsigned long buzzerStartTime = 0;
int maxDistance = 0;

// EEPROM data tracking
int eepromWriteAddress = 0; // Tracks the current EEPROM write position

// Servo object
Servo myServo;

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  myServo.attach(servoPin);
  myServo.write(0);
  digitalWrite(buzzerPin, LOW);

  Serial.println("Press 'r' to retrieve stored data or 'c' to clear EEPROM.");
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'r') {
      retrieveData();
    } else if (command == 'c') {
      clearEEPROM();
    }
  }

  int distance = getDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Buzzer and servo control logic
  if (!buzzerOn && distance >= activateMinDistance && distance <= activateMaxDistance) {
    buzzerOn = true;
    maxDistance = distance;         // Initialize max distance
    buzzerStartTime = millis();     // Record start time
    digitalWrite(buzzerPin, HIGH);
    myServo.write(90);              // Rotate to 90°
    Serial.println("Buzzer ON");
  } else if (buzzerOn && distance <= stopDistance) {
    buzzerOn = false;
    digitalWrite(buzzerPin, LOW);
    myServo.write(0);               // Reset to 0°

    // Calculate liquid volume
    int liquidVolume = calculateLiquidVolume(maxDistance, stopDistance);

    // Log data to EEPROM
    storeData(buzzerStartTime, liquidVolume);

    Serial.print("Buzzer Duration: ");
    Serial.print(millis() - buzzerStartTime);
    Serial.println(" ms");

    Serial.print("Liquid Volume: ");
    Serial.print(liquidVolume);
    Serial.println(" liters");
  } else if (buzzerOn && distance > maxDistance) {
    maxDistance = distance;
  }

  delay(100);
}

// Function to calculate distance from the ultrasonic sensor
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  return distance;
}

// Function to calculate liquid volume based on maximum distance and stop distance
int calculateLiquidVolume(int maxDistance, int stopDistance) {
  return (maxDistance - stopDistance) * 10;
}

// Function to store data into EEPROM
void storeData(unsigned long startTime, int liquidVolume) {
  if (eepromWriteAddress + 8 < EEPROM.length()) {
    // Store startTime (4 bytes)
    EEPROM.put(eepromWriteAddress, startTime);
    eepromWriteAddress += sizeof(startTime);

    // Store liquidVolume (4 bytes)
    EEPROM.put(eepromWriteAddress, liquidVolume);
    eepromWriteAddress += sizeof(liquidVolume);

    Serial.println("Data logged: Time=" + String(startTime) + " ms, Volume=" + String(liquidVolume) + " liters");
  } else {
    Serial.println("EEPROM Full! Cannot store more data.");
  }
}

// Function to retrieve stored data from EEPROM
void retrieveData() {
  Serial.println("Retrieving stored data:");
  int readAddress = 0;

  while (readAddress + 8 <= eepromWriteAddress) {
    unsigned long startTime;
    int liquidVolume;

    // Read startTime (4 bytes)
    EEPROM.get(readAddress, startTime);
    readAddress += sizeof(startTime);

    // Read liquidVolume (4 bytes)
    EEPROM.get(readAddress, liquidVolume);
    readAddress += sizeof(liquidVolume);

    // Display retrieved data
    Serial.print("Time: ");
    Serial.print(startTime);
    Serial.print(" ms, Volume: ");
    Serial.print(liquidVolume);
    Serial.println(" liters");
  }

  Serial.println("Data retrieval complete.");
}

// Function to clear EEPROM
void clearEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
  eepromWriteAddress = 0;
  Serial.println("EEPROM Cleared.");
}
