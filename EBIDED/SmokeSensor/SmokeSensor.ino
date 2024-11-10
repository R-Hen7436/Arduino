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

// MQ2 Sensor Pin Definitions
#define MQ2_DIGITAL_PIN D1  // Digital output pin for the MQ2 sensor
#define MQ2_ANALOG_PIN A0   // Analog output pin for the MQ2 sensor

// Smoke detection threshold
const int SMOKE_THRESHOLD = 300; // Example threshold for smoke concentration

void setup() {
  Serial.begin(115200);

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
  // Read the digital output (D0) to check if smoke is detected
  int smokeDetected = digitalRead(MQ2_DIGITAL_PIN);
  int smokeLevel = analogRead(MQ2_ANALOG_PIN);

  // Prepare messages
  String smokeStatus = (smokeDetected == LOW) ? "Smoke Detected" : "No Smoke Detected";
  
  // Update Firebase with smoke status
  if (Firebase.setString(firebaseData, "/test/Smoke", smokeStatus)) {
    Serial.println("Smoke status updated successfully: " + smokeStatus);
  } else {
    Serial.println("Failed to update smoke status");
    Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
  }

  // Update Firebase with smoke levels
  if (Firebase.setInt(firebaseData, "/test/SmokeLevel", smokeLevel)) {
    Serial.println("Smoke level updated successfully: " + String(smokeLevel));
  } else {
    Serial.println("Failed to update smoke levels");
    Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
  }

  delay(1000);  // Wait 1 second before reading again
}
