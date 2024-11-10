#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Your Wi-Fi credentials
#define WIFI_SSID "CabigonU"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Use for Wi-Fi password if required

// Your Firebase project credentials
#define FIREBASE_HOST "https://ebided-99644-default-rtdb.asia-southeast1.firebasedatabase.app" // Full URL
#define FIREBASE_AUTH "brJ0IYWr1LLAeaBDiXcNn90vw0zLN0Zwh9dSmnNL" // Firebase secret token

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; // Empty auth object
FirebaseConfig config;

// Pins for the lock, flame sensor, and smoke sensor
#define LOCK_PIN D2           // Pin connected to the lock
#define FLAME_SENSOR_PIN D1   // Pin connected to the flame sensor
#define MQ2_DIGITAL_PIN D3    // Digital output pin for the MQ2 smoke sensor
#define MQ2_ANALOG_PIN A0     // Analog output pin for the MQ2 smoke sensor

// Smoke detection threshold
const int SMOKE_THRESHOLD = 300; // Example threshold for smoke concentration

int previousValue = -1;  // To store the previous value from Firebase
bool flameDetected = false;  // To store the flame detection status

void setup() {
  Serial.begin(115200);

  // Initialize LOCK, FLAME_SENSOR, and MQ2_SENSOR pins
  pinMode(LOCK_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, LOW);  // Turn lock off initially
  pinMode(FLAME_SENSOR_PIN, INPUT);  // Set flame sensor pin as input
  pinMode(MQ2_DIGITAL_PIN, INPUT);   // Set smoke sensor digital pin as input

  // Start Wi-Fi connection
  Serial.println("Starting Wi-Fi connection...");
  WiFi.begin(WIFI_SSID); // Connect without a password for open networks

  // Wi-Fi connection timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 15000) {  // 15-second timeout
      Serial.println("Failed to connect to Wi-Fi.");
      Serial.println("Wi-Fi Status: " + String(WiFi.status()));
      return;  // Exit setup if Wi-Fi fails to connect
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.database_url = FIREBASE_HOST; // Ensure you provide the full URL with https://
  config.signer.tokens.legacy_token = FIREBASE_AUTH; // Use the legacy token (your Firebase secret)

  // Initialize Firebase with config and empty auth object
  Firebase.begin(&config, &auth); 
  Firebase.reconnectWiFi(true);

  // Check Firebase connection
  if (!Firebase.ready()) {
    Serial.println("Failed to connect to Firebase.");
    Serial.println(firebaseData.errorReason()); // Print more details on the failure
    return;
  }
  Serial.println("Connected to Firebase");
}

void checkFlameStatus() {
  int sensorValue = digitalRead(FLAME_SENSOR_PIN);  // Read from flame sensor

  // If sensor detects flame, sensorValue might be LOW (depends on sensor type)
  bool newFlameDetected = (sensorValue == LOW);  // Adjust based on your sensor's logic

  if (newFlameDetected != flameDetected) {  // Only update if there's a change in status
    flameDetected = newFlameDetected;

    // Update Firebase with the flame detection status
    String status = flameDetected ? "Flame Detected" : "No Flame Detected";
    if (Firebase.setString(firebaseData, "/test/Flame", status)) {
      Serial.print("Flame Status Updated: ");
      Serial.println(status);
    } else {
      Serial.println("Failed to update flame status.");
      Serial.println(firebaseData.errorReason());
    }
  }
}

void checkSmokeStatus() {
  int smokeDetected = digitalRead(MQ2_DIGITAL_PIN);  // Read digital output for smoke detection
  int smokeLevel = analogRead(MQ2_ANALOG_PIN);       // Read analog output for smoke level

  // Determine smoke status based on digital reading
  String smokeStatus = (smokeDetected == LOW) ? "Smoke Detected" : "No Smoke Detected";

  // Update Firebase with smoke status
  if (Firebase.setString(firebaseData, "/test/Smoke", smokeStatus)) {
    Serial.println("Smoke status updated successfully: " + smokeStatus);
  } else {
    Serial.println("Failed to update smoke status");
    Serial.println(firebaseData.errorReason());
  }

  // Update Firebase with smoke level
  if (Firebase.setInt(firebaseData, "/test/SmokeLevel", smokeLevel)) {
    Serial.println("Smoke level updated successfully: " + String(smokeLevel));
  } else {
    Serial.println("Failed to update smoke level");
    Serial.println(firebaseData.errorReason());
  }
}

void loop() {
  // Read the value from Firebase
  if (Firebase.getInt(firebaseData, "/test/value")) {
    int currentValue = firebaseData.intData(); // Get the integer value

    // Check if value is different than the previous one
    if (currentValue != previousValue) {
      Serial.print("Firebase value: ");
      Serial.println(currentValue);

      // Control the lock based on the value
      if (currentValue == 1) {
        digitalWrite(LOCK_PIN, HIGH);  // Unlock
        Serial.println("LOCK UNLOCKED");
      } else if (currentValue == 0) {
        digitalWrite(LOCK_PIN, LOW);  // Lock
        Serial.println("LOCK LOCKED");
      }

      // Update the previous value
      previousValue = currentValue;
    }
  } else {
    Serial.println("Failed to read value");
    Serial.println(firebaseData.errorReason()); // Print the error reason for the failed read
  }

  // Check flame and smoke statuses
  checkFlameStatus();
  checkSmokeStatus();

  delay(1000);  // Delay to reduce the number of reads (adjust this as needed)
}
