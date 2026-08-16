#include <Keypad.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// ======================================================
// LCD
// ======================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======================================================
// SERVO
// ======================================================

Servo entryGate;
Servo exitGate;

// ======================================================
// OUTPUT PINS
// ======================================================

const int buzzer   = 8;
const int greenLED = 9;
const int redLED   = 10;
const int blueLED  = 11;

// ======================================================
// SERVO PINS
// ======================================================

const int entryServoPin = 5;
const int exitServoPin  = 12;

// ======================================================
// ULTRASONIC
// ======================================================

const int trigPin = 47;
const int echoPin = 46;

// ======================================================
// RFID RC522
// Arduino Mega SPI pins:
// MISO = 50
// MOSI = 51
// SCK  = 52
// SS   = 53
// ======================================================

#define RFID_SS_PIN   53
#define RFID_RST_PIN  49

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// ======================================================
// PARKING
// ======================================================

int totalCapacity = 10;
int totalEntry    = 0;
int totalExit     = 0;

// ======================================================
// KEYPAD
// ======================================================

const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {30, 31, 32, 33};
byte colPins[COLS] = {34, 35, 36};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);

// ======================================================
// USER STRUCTURE
// ======================================================

struct User {
  String pin;
  String name;
  String role;
  String plate;
  String type;
  String rfidUID;
  bool isInside;
  bool active;
};

// ======================================================
// MAX USERS
// ======================================================

const int MAX_USERS = 15;

// ======================================================
// REGISTERED USERS
// IMPORTANT: Replace example RFID UIDs with real UIDs.
// ======================================================

User users[MAX_USERS] = {

  {
    "12345",
    "Dr.Monir Sir",
    "Teacher",
    "TCH-101",
    "Car",
    "A1B2C3D4",
    false,
    true
  },

  {
    "56789",
    "Dr.Shahin Sir",
    "Teacher",
    "TCH-102",
    "Bike",
    "11223344",
    false,
    true
  },

  {
    "56780",
    "Dr. Sajjad Sir",
    "Teacher",
    "TCH-103",
    "Microbus",
    "55667788",
    false,
    true
  },

  {
    "77777",
    "Kawsar Ahmed",
    "Guest Teacher",
    "DHAKA-1122",
    "Car",
    "12345678",
    false,
    true
  },

  {
    "88888",
    "Dr. Kabir",
    "External Examiner",
    "RAJ-3344",
    "Car",
    "ABCDEF12",
    false,
    true
  },

  {
    "77778",
    "Antor",
    "Student",
    "JH-3344",
    "MotorCycle",
    "99887766",
    false,
    true
  },

  {
    "13456",
    "Ashikur",
    "Staff",
    "RAJ-3454",
    "Bike",
    "DEADBEEF",
    false,
    true
  }
};

// ======================================================
// GUEST
// ======================================================

int guestCounter = 1;

// ======================================================
// PIN
// ======================================================

String inputPIN = "";

// ======================================================
// ADMIN
// ======================================================

const String ADMIN_PIN = "9999";

bool isAdminMode = false;

// ======================================================
// SECURITY
// ======================================================

int wrongAttempts = 0;

unsigned long lastKeyPressTime = 0;

const unsigned long INPUT_TIMEOUT = 15000;

// ======================================================
// DUPLICATE PROTECTION
// ======================================================

String lastProcessedPIN = "";

unsigned long lastProcessedTime = 0;

const unsigned long DOUBLE_ENTRY_COOLDOWN = 5000;

String lastProcessedRFID = "";

unsigned long lastRFIDTime = 0;

const unsigned long RFID_COOLDOWN = 5000;

// ======================================================
// VEHICLE STATE
// ======================================================

bool lastCarState = false;

// ======================================================
// FUNCTION PROTOTYPES
// ======================================================

long getDistance();

void processPIN();
void processRFID();

void showIdleScreen();

void sendVehicleDataToESP32(User u, String accessType);

void handleAdminMenu();
void addNewGuest();
void deleteUser();

void playKeyPressSound();
void playWrongPinSound();

String getRFIDUID();

int findUserByRFID(String uid);
int findUserByPIN(String pin);

void processAuthenticatedUser(User &u);

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  // Mega Serial1 -> ESP32
  Serial1.begin(9600);

  // LCD
  lcd.init();
  lcd.backlight();

  // Outputs
  pinMode(buzzer, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Servo
  entryGate.attach(entryServoPin);
  exitGate.attach(exitServoPin);

  // Initial gate positions
  entryGate.write(80);
  exitGate.write(0);

  // LEDs OFF
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  delay(100);

  // Startup
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vehicle Access");
  lcd.setCursor(0, 1);
  lcd.print("RFID + PIN");

  delay(2000);

  lcd.clear();
  showIdleScreen();

  Serial.println("================================");
  Serial.println("Smart Vehicle Access System");
  Serial.println("RFID + PIN System");
  Serial.println("RFID Reader Ready");
  Serial.println("================================");
}

// ======================================================
// KEY PRESS SOUND
// ======================================================

void playKeyPressSound() {

  tone(buzzer, 3000);
  delay(100);
  noTone(buzzer);
}

// ======================================================
// WRONG PIN / CARD SOUND
// ======================================================

void playWrongPinSound() {

  for (int i = 0; i < 3; i++) {

    tone(buzzer, 3500);
    delay(200);

    tone(buzzer, 1500);
    delay(200);
  }

  noTone(buzzer);
}

// ======================================================
// DISTANCE
// ======================================================

long getDistance() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(
    echoPin,
    HIGH,
    15000
  );

  if (duration == 0) {
    return 999;
  }

  return duration * 0.034 / 2;
}

// ======================================================
// GET RFID UID
// ======================================================

String getRFIDUID() {

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(
      rfid.uid.uidByte[i],
      HEX
    );
  }

  uid.toUpperCase();

  return uid;
}

// ======================================================
// FIND USER BY RFID
// ======================================================

int findUserByRFID(String uid) {

  uid.toUpperCase();

  for (int i = 0; i < MAX_USERS; i++) {

    if (
      users[i].active &&
      users[i].rfidUID == uid
    ) {
      return i;
    }
  }

  return -1;
}

// ======================================================
// FIND USER BY PIN
// ======================================================

int findUserByPIN(String pin) {

  for (int i = 0; i < MAX_USERS; i++) {

    if (
      users[i].active &&
      users[i].pin == pin
    ) {
      return i;
    }
  }

  return -1;
}

// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  long distance = getDistance();

  bool carPresent =
    (distance > 0 && distance < 6);

  // ==================================================
  // NO VEHICLE
  // ==================================================

  if (!carPresent) {

    if (lastCarState != false) {

      inputPIN = "";

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("    Waiting...  ");

      lcd.setCursor(0, 1);
      lcd.print("  For Vehicle   ");

      digitalWrite(blueLED, HIGH);
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, LOW);

      lastCarState = false;
    }

    lastKeyPressTime = millis();
  }

  // ==================================================
  // VEHICLE ARRIVED
  // ==================================================

  else {

    if (lastCarState != true) {

      lcd.clear();
      showIdleScreen();

      digitalWrite(blueLED, LOW);
      digitalWrite(greenLED, HIGH);
      digitalWrite(redLED, LOW);

      lastCarState = true;
      lastKeyPressTime = millis();
    }

    // PIN timeout
    if (
      inputPIN.length() > 0 &&
      millis() - lastKeyPressTime > INPUT_TIMEOUT
    ) {

      inputPIN = "";

      lcd.clear();

      lcd.setCursor(0, 1);
      lcd.print("TIMEOUT CLEAR!");

      delay(1000);

      showIdleScreen();
    }

    // =================================================
    // RFID
    // =================================================

    if (
      rfid.PICC_IsNewCardPresent() &&
      rfid.PICC_ReadCardSerial()
    ) {

      processRFID();

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();

      delay(500);
    }

    // =================================================
    // KEYPAD
    // =================================================

    char key = keypad.getKey();

    if (key) {

      lastKeyPressTime = millis();

      playKeyPressSound();

      // Submit
      if (key == '#') {

        if (inputPIN.length() > 0) {

          processPIN();

          inputPIN = "";

          if (
            !isAdminMode &&
            lastCarState
          ) {
            showIdleScreen();
          }
        }
      }

      // Clear
      else if (key == '*') {

        inputPIN = "";

        showIdleScreen();
      }

      // Number
      else {

        if (inputPIN.length() < 5) {

          inputPIN += key;

          lcd.setCursor(0, 0);
          lcd.print("Enter PIN:      ");

          lcd.setCursor(0, 1);
          lcd.print("                ");

          lcd.setCursor(0, 1);

          for (
            unsigned int i = 0;
            i < inputPIN.length();
            i++
          ) {
            lcd.print("*");
          }
        }
      }
    }
  }

  delay(10);
}

// ======================================================
// PROCESS PIN
// ======================================================

void processPIN() {

  // Admin
  if (inputPIN == ADMIN_PIN) {

    wrongAttempts = 0;

    handleAdminMenu();

    return;
  }

  // Duplicate PIN
  if (
    inputPIN == lastProcessedPIN &&
    millis() - lastProcessedTime <
    DOUBLE_ENTRY_COOLDOWN
  ) {

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("PLEASE WAIT...");

    lcd.setCursor(0, 1);
    lcd.print("Duplicate Input");

    delay(1500);

    return;
  }

  int userIndex =
    findUserByPIN(inputPIN);

  // Wrong PIN
  if (userIndex == -1) {

    wrongAttempts++;

    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("   WRONG PIN!   ");

    lcd.setCursor(0, 1);
    lcd.print(
      "Attempts: " +
      String(wrongAttempts) +
      "/3"
    );

    playWrongPinSound();

    if (wrongAttempts >= 3) {

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print(" SYSTEM LOCKED! ");

      for (int i = 15; i > 0; i--) {

        lcd.setCursor(0, 1);

        lcd.print(
          "Wait: " +
          String(i) +
          "s        "
        );

        tone(buzzer, 3500);
        delay(200);

        noTone(buzzer);
        delay(800);
      }

      wrongAttempts = 0;
    }

    lcd.clear();

    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);

    return;
  }

  // Valid PIN
  wrongAttempts = 0;

  lastProcessedPIN = inputPIN;
  lastProcessedTime = millis();

  User &u = users[userIndex];

  processAuthenticatedUser(u);
}

// ======================================================
// PROCESS RFID
// ======================================================

void processRFID() {

  String uid = getRFIDUID();

  Serial.print("RFID UID: ");
  Serial.println(uid);

  // Duplicate RFID
  if (
    uid == lastProcessedRFID &&
    millis() - lastRFIDTime <
    RFID_COOLDOWN
  ) {

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("PLEASE WAIT...");

    lcd.setCursor(0, 1);
    lcd.print("Card Already Used");

    delay(1500);

    showIdleScreen();

    return;
  }

  int userIndex =
    findUserByRFID(uid);

  // Unknown card
  if (userIndex == -1) {

    wrongAttempts++;

    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(" UNKNOWN CARD!  ");

    lcd.setCursor(0, 1);
    lcd.print(
      "Attempts: " +
      String(wrongAttempts) +
      "/3"
    );

    playWrongPinSound();

    if (wrongAttempts >= 3) {

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print(" SYSTEM LOCKED! ");

      for (int i = 15; i > 0; i--) {

        lcd.setCursor(0, 1);

        lcd.print(
          "Wait: " +
          String(i) +
          "s        "
        );

        tone(buzzer, 3500);
        delay(200);

        noTone(buzzer);
        delay(800);
      }

      wrongAttempts = 0;
    }

    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);

    showIdleScreen();

    return;
  }

  // Valid RFID
  wrongAttempts = 0;

  lastProcessedRFID = uid;
  lastRFIDTime = millis();

  User &u = users[userIndex];

  processAuthenticatedUser(u);
}

// ======================================================
// AUTHENTICATED USER
// ======================================================

void processAuthenticatedUser(User &u) {

  // ==================================================
  // ENTRY
  // ==================================================

  if (!u.isInside) {

    u.isInside = true;

    totalEntry++;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Role: ");
    lcd.print(u.role);

    lcd.setCursor(0, 1);
    lcd.print("Welcome ");
    lcd.print(u.name);

    sendVehicleDataToESP32(
      u,
      "Entry"
    );

    delay(1500);

    entryGate.write(80);

    delay(500);

    entryGate.write(0);

    delay(5000);

    entryGate.write(80);
  }

  // ==================================================
  // EXIT
  // ==================================================

  else {

    u.isInside = false;

    totalExit++;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Role: ");
    lcd.print(u.role);

    lcd.setCursor(0, 1);
    lcd.print("Goodbye ");
    lcd.print(u.name);

    sendVehicleDataToESP32(
      u,
      "Exit"
    );

    delay(1500);

    exitGate.write(90);

    delay(5000);

    exitGate.write(0);
  }

  lcd.clear();

  showIdleScreen();
}

// ======================================================
// ADMIN MENU
// ======================================================

void handleAdminMenu() {

  isAdminMode = true;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ADMIN MODE");

  lcd.setCursor(0, 1);
  lcd.print("1:Add 2:Delete");

  char option = 0;

  while (
    option != '1' &&
    option != '2' &&
    option != '*'
  ) {

    option = keypad.getKey();

    if (option) {
      playKeyPressSound();
    }
  }

  if (option == '1') {
    addNewGuest();
  }

  else if (option == '2') {
    deleteUser();
  }

  isAdminMode = false;

  lcd.clear();
  lcd.print("Exiting Admin...");

  delay(1000);
}

// ======================================================
// ADD NEW GUEST
// ======================================================

void addNewGuest() {

  int emptyIndex = -1;

  for (int i = 0; i < MAX_USERS; i++) {

    if (!users[i].active) {

      emptyIndex = i;
      break;
    }
  }

  if (emptyIndex == -1) {

    lcd.clear();
    lcd.print("User Limit Full!");

    delay(2000);

    return;
  }

  String newPin = "";

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Guest PIN + #");

  while (true) {

    char k = keypad.getKey();

    if (k) {

      playKeyPressSound();

      if (k == '#') {

        if (newPin.length() == 4) {
          break;
        }

        else {

          lcd.clear();
          lcd.print("Must be 4 Digits");

          delay(1500);

          return;
        }
      }

      else if (k == '*') {
        return;
      }

      else if (
        k >= '0' &&
        k <= '9'
      ) {

        if (newPin.length() < 4) {

          newPin += k;

          lcd.setCursor(0, 1);
          lcd.print(newPin);
        }
      }
    }
  }

  users[emptyIndex].pin = newPin;

  users[emptyIndex].name =
    "Guest-" +
    String(guestCounter);

  users[emptyIndex].role =
    "Visitor";

  users[emptyIndex].plate =
    "GST-" +
    String(100 + guestCounter);

  users[emptyIndex].type =
    "Car";

  users[emptyIndex].rfidUID = "";

  users[emptyIndex].isInside = false;

  users[emptyIndex].active = true;

  guestCounter++;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Guest Added!");

  lcd.setCursor(0, 1);
  lcd.print("PIN: ");
  lcd.print(newPin);

  delay(2000);
}

// ======================================================
// DELETE USER
// ======================================================

void deleteUser() {

  String delPin = "";

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Del PIN + (#)");

  while (true) {

    char k = keypad.getKey();

    if (k) {

      playKeyPressSound();

      if (k == '#') {

        if (
          delPin.length() == 4 ||
          delPin.length() == 5
        ) {
          break;
        }

        else {

          lcd.clear();
          lcd.print("Invalid PIN Len!");

          delay(1500);

          return;
        }
      }

      else if (k == '*') {
        return;
      }

      else if (
        k >= '0' &&
        k <= '9'
      ) {

        if (delPin.length() < 5) {

          delPin += k;

          lcd.setCursor(0, 1);
          lcd.print(delPin);
        }
      }
    }
  }

  bool found = false;

  for (int i = 0; i < MAX_USERS; i++) {

    if (
      users[i].active &&
      users[i].pin == delPin
    ) {

      users[i].active = false;

      users[i].isInside = false;

      users[i].rfidUID = "";

      found = true;

      break;
    }
  }

  lcd.clear();

  if (found) {
    lcd.print("PIN Deleted!");
  }

  else {
    lcd.print("PIN Not Found!");
  }

  delay(2000);
}

// ======================================================
// IDLE SCREEN
// ======================================================

void showIdleScreen() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("PIN / RFID");

  lcd.setCursor(0, 1);
  lcd.print("Scan / Enter");
}

// ======================================================
// SEND DATA TO ESP32
// ======================================================

void sendVehicleDataToESP32(
  User u,
  String accessType
) {

  int insideCount = 0;

  for (int i = 0; i < MAX_USERS; i++) {

    if (
      users[i].active &&
      users[i].isInside
    ) {

      insideCount++;
    }
  }

  String jsonPayload =
    "{\"total\":" +
    String(totalCapacity) +

    ",\"inside\":" +
    String(insideCount) +

    ",\"entry\":" +
    String(totalEntry) +

    ",\"exit\":" +
    String(totalExit) +

    ",\"latest_vehicle\":{" +

    "\"name\":\"" +
    u.name +

    "\"," +

    "\"plate\":\"" +
    u.plate +

    "\"," +

    "\"type\":\"" +
    u.type +

    "\"," +

    "\"status\":\"" +
    (u.isInside ? "Inside" : "Outside") +

    "\"," +

    "\"access\":\"" +
    accessType +

    "\"," +

    "\"category\":\"" +
    u.role +

    "\"" +

    "}}\n";

  Serial1.print(jsonPayload);

  Serial.println("Sent to ESP32:");
  Serial.println(jsonPayload);
}
