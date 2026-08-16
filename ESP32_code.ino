#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

// ======================================================
// WiFi Configuration
// ======================================================
// Add your own WiFi credentials before uploading.
// Do NOT commit real credentials to a public GitHub repo.

#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ======================================================
// Firebase Configuration
// ======================================================
// Add your own Firebase Realtime Database host/token.
// Do NOT commit private credentials to a public repository.

#define FIREBASE_HOST "YOUR_FIREBASE_DATABASE_URL"
#define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_TOKEN"

// ======================================================
// Firebase Objects
// ======================================================

FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// ======================================================
// Serial Communication
// Arduino Mega TX1/RX1 <-> ESP32 RX2/TX2
// ======================================================

#define RXD2 16
#define TXD2 17

// ======================================================
// NTP
// Bangladesh UTC+6
// ======================================================

WiFiUDP ntpUDP;

NTPClient timeClient(
  ntpUDP,
  "pool.ntp.org",
  6 * 3600,
  60000
);

// ======================================================
// Date + Time
// ======================================================

String getDateTime() {

  time_t rawTime =
    timeClient.getEpochTime();

  struct tm *ptm =
    localtime(&rawTime);

  int hour =
    ptm->tm_hour;

  String ampm = "AM";

  if (hour >= 12) {

    ampm = "PM";

    if (hour > 12) {
      hour -= 12;
    }
  }

  if (hour == 0) {
    hour = 12;
  }

  char buffer[40];

  sprintf(
    buffer,
    "%04d-%02d-%02d %02d:%02d:%02d %s",

    ptm->tm_year + 1900,
    ptm->tm_mon + 1,
    ptm->tm_mday,

    hour,
    ptm->tm_min,
    ptm->tm_sec,

    ampm.c_str()
  );

  return String(buffer);
}

// ======================================================
// WiFi Connection
// ======================================================

void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting WiFi");

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  unsigned long startTime =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 20000
  ) {

    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("WiFi Connected!");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }

  else {

    Serial.println();
    Serial.println(
      "WiFi connection failed."
    );
  }
}

// ======================================================
// Firebase Setup
// ======================================================

void setupFirebase() {

  config.host =
    FIREBASE_HOST;

  config.signer.tokens.legacy_token =
    FIREBASE_AUTH;

  Firebase.begin(
    &config,
    &auth
  );

  Firebase.reconnectWiFi(true);

  Serial.println(
    "Firebase initialized."
  );
}

// ======================================================
// Setup
// ======================================================

void setup() {

  Serial.begin(115200);

  // Mega -> ESP32 communication
  Serial2.begin(
    9600,
    SERIAL_8N1,
    RXD2,
    TXD2
  );

  Serial2.setTimeout(100);

  // WiFi
  connectWiFi();

  // Firebase
  if (WiFi.status() == WL_CONNECTED) {
    setupFirebase();
  }

  // NTP
  timeClient.begin();

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }

  Serial.println(
    "ESP32 Firebase Gateway Ready"
  );
}

// ======================================================
// Update Firebase Overview
// ======================================================

void updateOverview(
  JsonDocument &doc
) {

  FirebaseJson overviewJson;

  overviewJson.set(
    "total_capacity",
    doc["total"].as<int>()
  );

  overviewJson.set(
    "inside",
    doc["inside"].as<int>()
  );

  overviewJson.set(
    "total_entry",
    doc["entry"].as<int>()
  );

  overviewJson.set(
    "total_exit",
    doc["exit"].as<int>()
  );

  if (
    Firebase.updateNode(
      firebaseData,
      "/overview",
      overviewJson
    )
  ) {

    Serial.println(
      "Overview Updated"
    );
  }

  else {

    Serial.print(
      "Overview Error: "
    );

    Serial.println(
      firebaseData.errorReason()
    );
  }
}

// ======================================================
// Send Vehicle Log
// ======================================================

void sendVehicleLog(
  JsonObject vehicle,
  String currentDateTime
) {

  FirebaseJson vehicleJson;

  vehicleJson.set(
    "name",
    vehicle["name"].as<String>()
  );

  vehicleJson.set(
    "vehicle_no",
    vehicle["plate"].as<String>()
  );

  vehicleJson.set(
    "type",
    vehicle["type"].as<String>()
  );

  vehicleJson.set(
    "category",
    vehicle["category"].as<String>()
  );

  vehicleJson.set(
    "access",
    vehicle["access"].as<String>()
  );

  vehicleJson.set(
    "status",
    vehicle["status"].as<String>()
  );

  // Date and time from NTP
  vehicleJson.set(
    "timestamp",
    currentDateTime
  );

  // RFID/PIN authentication method
  if (
    vehicle.containsKey("method")
  ) {

    vehicleJson.set(
      "method",
      vehicle["method"].as<String>()
    );
  }

  if (
    Firebase.pushJSON(
      firebaseData,
      "/vehicle_logs",
      vehicleJson
    )
  ) {

    Serial.print(
      "Vehicle log sent: "
    );

    Serial.println(
      currentDateTime
    );
  }

  else {

    Serial.print(
      "Firebase Log Error: "
    );

    Serial.println(
      firebaseData.errorReason()
    );

    // One retry
    Firebase.pushJSON(
      firebaseData,
      "/vehicle_logs",
      vehicleJson
    );
  }
}

// ======================================================
// Process Mega JSON
// ======================================================

void processMegaData(
  String jsonString
) {

  Serial.println(
    "Received from Mega:"
  );

  Serial.println(
    jsonString
  );

  // ==================================================
  // JSON Document
  // ==================================================

  StaticJsonDocument<1024> doc;

  DeserializationError error =
    deserializeJson(
      doc,
      jsonString
    );

  if (error) {

    Serial.print(
      "JSON Parse Error: "
    );

    Serial.println(
      error.c_str()
    );

    return;
  }

  // ==================================================
  // Safety Check
  // ==================================================

  if (
    !doc.containsKey("latest_vehicle")
  ) {

    Serial.println(
      "Invalid JSON: latest_vehicle missing"
    );

    return;
  }

  // ==================================================
  // Update Overview
  // ==================================================

  updateOverview(doc);

  // ==================================================
  // Current Date + Time
  // ==================================================

  if (WiFi.status() == WL_CONNECTED) {

    if (!timeClient.update()) {
      timeClient.forceUpdate();
    }
  }

  String currentDateTime =
    getDateTime();

  // ==================================================
  // Vehicle Object
  // ==================================================

  JsonObject vehicle =
    doc["latest_vehicle"];

  // ==================================================
  // Send Log
  // ==================================================

  sendVehicleLog(
    vehicle,
    currentDateTime
  );
}

// ======================================================
// Main Loop
// ======================================================

void loop() {

  // ==================================================
  // WiFi Auto Reconnect
  // ==================================================

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    connectWiFi();

    if (
      WiFi.status() == WL_CONNECTED
    ) {

      setupFirebase();

      timeClient.update();
    }
  }

  // ==================================================
  // Receive Mega Data
  // ==================================================

  if (Serial2.available()) {

    String jsonString =
      Serial2.readStringUntil('\n');

    jsonString.trim();

    if (jsonString.length() == 0) {
      return;
    }

    // ==================================================
    // Duplicate Protection
    // ==================================================

    static String lastData = "";

    if (
      jsonString == lastData
    ) {

      Serial.println(
        "Duplicate data ignored."
      );

      return;
    }

    lastData =
      jsonString;

    // ==================================================
    // Firebase
    // ==================================================

    if (
      WiFi.status() != WL_CONNECTED
    ) {

      Serial.println(
        "No WiFi. Data not uploaded."
      );

      return;
    }

    if (!Firebase.ready()) {

      Serial.println(
        "Firebase not ready."
      );

      return;
    }

    processMegaData(
      jsonString
    );
  }

  delay(10);
}
