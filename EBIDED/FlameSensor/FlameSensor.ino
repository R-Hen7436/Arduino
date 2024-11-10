#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Your Wi-Fi credentials
#define WIFI_SSID "CabigonU"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Use for Wi-Fi password if required

// Your Firebase project credentials
#define FIREBASE_HOST "https://ebided-99644-default-rtdb.asia-southeast1.firebasedatabase.app" // Full URL
#define FIREBASE_AUTH "brJ0IYWr1LLAeaBDiXcNn90vw0zLN0Zwh9dSmnNL" // Firebase secret token

// Flame sensor pin
#define FLAME_SENSOR_PIN D1

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; // Empty auth object
FirebaseConfig config;

// Variable to store the last detected state
String lastState = "";

void setup() {
  Serial.begin(115200);

  // Set up flame sensor pin as input
  pinMode(FLAME_SENSOR_PIN, INPUT);

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
  // Read the flame sensor state
  int flameState = digitalRead(FLAME_SENSOR_PIN);

  String Flame;
  if (flameState == LOW) {
    Flame = "Flame Detected";  // Flame detected (sensor sends LOW when flame is detected)
  } else {
    Flame = "No Flame Detected";  // No flame detected
  }

  // Send Flame to Firebase only if there's a state change
  if (Flame != lastState) {
    if (Firebase.setString(firebaseData, "/test/Flame", Flame)) {
      Serial.print("Flame sent: ");
      Serial.println(Flame);
    } else {
      Serial.print("Failed to send Flame: ");
      Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
    }
    lastState = Flame;  // Update last state to the current state
  }

  delay(1000);  // Check every second
}
