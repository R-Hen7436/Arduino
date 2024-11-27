#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Your Wi-Fi credentials
#define WIFI_SSID "UbasC"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Use for Wi-Fi password if required

// Your Firebase project credentials
#define FIREBASE_HOST "https://smartlock-46110-default-rtdb.asia-southeast1.firebasedatabase.app" // Full URL
#define FIREBASE_AUTH "meqpOYjcPQLL2CtuiFIgRUM4YQjU6KJSC4zAqs7E" // Firebase secret token

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; // Empty auth object
FirebaseConfig config;
  
// Pin connected to the solenoid lock (Relay module)
#define SOLENOID_PIN D5  // Use GPIO pin D2 (adjust as needed)

int previousValue = -1;  // To store the previous value from Firebase

void setup() {
  Serial.begin(115200);

  // Initialize solenoid pin
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);  // Keep the lock initially in the locked position (LOW)

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

void loop() {
  // Read the value from Firebase
  if (Firebase.getInt(firebaseData, "/sensors/LockStatus")) {
    int currentValue = firebaseData.intData(); // Get the integer value

    // Check if value is different than the previous one
    if (currentValue != previousValue) {
      Serial.print("Firebase value: ");
      Serial.println(currentValue);

      // Control the solenoid lock based on the value
      if (currentValue == 1) {
        digitalWrite(SOLENOID_PIN, HIGH);  // Unlock the solenoid lock
        Serial.println("LOCK LOCKED");
      } else if (currentValue == 0) {
        digitalWrite(SOLENOID_PIN, LOW);  // Lock the solenoid lock
        Serial.println("LOCK UNLOCKED");
      }

      // Update the previous value
      previousValue = currentValue;
    }
  } else {
    Serial.println("Failed to read value");
    Serial.println(firebaseData.errorReason()); // Print the error reason for the failed read
  }

  delay(1000);  // Delay to reduce the number of reads (adjust this as needed)
}
