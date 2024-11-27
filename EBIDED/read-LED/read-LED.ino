#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Your Wi-Fi credentials
#define WIFI_SSID "UbasC"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Use for Wi-Fi password if required

// Your Firebase project credentials
#define FIREBASE_HOST "https://ebided-99644-default-rtdb.asia-southeast1.firebasedatabase.app" // Full URL
#define FIREBASE_AUTH "brJ0IYWr1LLAeaBDiXcNn90vw0zLN0Zwh9dSmnNL" // Firebase secret token

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; // Empty auth object
FirebaseConfig config;

// Pin connected to the LED
#define LED_PIN D2  // Use GPIO pin D2 (adjust as needed)

int previousValue = -1;  // To store the previous value from Firebase

void setup() {
  Serial.begin(115200);

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Turn LED off initially

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
  if (Firebase.getInt(firebaseData, "/test/value")) {
    int currentValue = firebaseData.intData(); // Get the integer value

    // Check if value is different than the previous one
    if (currentValue != previousValue) {
      Serial.print("Firebase value: ");
      Serial.println(currentValue);

      // Control the LED based on the value
      if (currentValue == 1) {
        digitalWrite(LED_PIN, HIGH);  // Turn the LED on
        Serial.println("LED ON");
      } else if (currentValue == 0) {
        digitalWrite(LED_PIN, LOW);  // Turn the LED off
        Serial.println("LED OFF");
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
