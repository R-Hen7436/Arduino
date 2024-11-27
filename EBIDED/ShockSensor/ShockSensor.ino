#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Wi-Fi credentials
#define WIFI_SSID "UbasC"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**"  // Uncomment if using a password

// Firebase project credentials
#define FIREBASE_HOST "https://smartlock-46110-default-rtdb.asia-southeast1.firebasedatabase.app" // Full URL
#define FIREBASE_AUTH "meqpOYjcPQLL2CtuiFIgRUM4YQjU6KJSC4zAqs7E" // Firebase secret token

// Sensor pin
#define VIBRATION_SENSOR_PIN D1

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;  // Empty auth object
FirebaseConfig config;

String lastState = "";
unsigned long timerStart = 0;       // Start time of the 10-second timer
unsigned long lastShockTime = 0;    // Last time a shock was detected
const unsigned long timerDuration = 15000;  // 15-second timer duration
const unsigned long noShockDuration = 5000; // 5 seconds of no shock
const unsigned long resetDelay = 3000;      // Delay after sending message (3 seconds)
bool isTimerRunning = false;        // Flag to check if the timer is running

void setup() {
  Serial.begin(115200);

  // Initialize sensor pin
  pinMode(VIBRATION_SENSOR_PIN, INPUT);

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID);
  
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
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Check Firebase connection
  if (!Firebase.ready()) {
    Serial.println("Failed to connect to Firebase.");
    Serial.println(firebaseData.errorReason());
    return;
  }
  Serial.println("Connected to Firebase");
}

void loop() {
  int shockState = digitalRead(VIBRATION_SENSOR_PIN);

  // Start the timer if a shock is detected and timer isn't already running
  if (shockState == HIGH && !isTimerRunning) {
    isTimerRunning = true;
    timerStart = millis();
    lastShockTime = millis();
    Serial.println("Shock detected. Timer started.");
  }

  // If timer is running, check for shock or no shock conditions
  if (isTimerRunning) {
    unsigned long currentTime = millis();

    if (shockState == HIGH) {
      lastShockTime = currentTime;  // Update last shock time on detection
    }

    // If no shock for 5 seconds, reset timer
    if (currentTime - lastShockTime >= noShockDuration) {
      Serial.println("No shock for 5 seconds. Resetting timer.");
      isTimerRunning = false;
      delay(resetDelay);  // Pause for 3 seconds before resuming monitoring
    }

    // If timer reaches 10 seconds and shock is still detected, send "Shock Detected"
    else if (currentTime - timerStart >= timerDuration) {
      if (lastState != "Shock Detected") {
        if (Firebase.setString(firebaseData, "/users/Alren/sensors/shockStatus", "Shock Detected")) {
          Serial.println("Shock Detected - Message sent to Firebase.");
        } else {
          Serial.print("Failed to send message: ");
          Serial.println(firebaseData.errorReason());
        }
        lastState = "Shock Detected";
      }
      
      // Reset timer and wait 3 seconds before checking for new shocks
      isTimerRunning = false;
      delay(resetDelay);

      // Send "No Shock Detected" after delay to reset state in Firebase
      if (Firebase.setString(firebaseData, "/users/Alren/sensors/shockStatus", "No Shock Detected")) {
        Serial.println("No Shock Detected - Reset message sent to Firebase.");
      } else {
        Serial.print("Failed to send message: ");
        Serial.println(firebaseData.errorReason());
      }
      lastState = "No Shock Detected";
    }
  }

  delay(100);  // Short delay for stable detection
}
