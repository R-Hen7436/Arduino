#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Wi-Fi credentials
#define WIFI_SSID "UbasC"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Uncomment and set your Wi-Fi password if required

// Firebase credentials
#define FIREBASE_HOST "https://smartlock-46110-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "meqpOYjcPQLL2CtuiFIgRUM4YQjU6KJSC4zAqs7E"

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;    

// Sensor pin
#define VIBRATION_SENSOR_PIN D2
int shockState = 0;

// Flame sensor pin
#define FLAME_SENSOR_PIN D5
String F_lastState = "";

// MQ2 Sensor Pin Definitions
#define MQ2_ANALOG_PIN A0   // Analog output pin for the MQ2 sensor
// Default smoke threshold if not found in Firebase
int SMOKE_THRESHOLD = 500; 

#define BUZZER_PIN D8  // Connect the buzzer to pin D5

String lastState = "";
unsigned long timerStart = 0;       // Start time of the 10-second timer
unsigned long lastShockTime = 0;    // Last time a shock was detected
const unsigned long timerDuration = 5000;  // 15-second timer duration
const unsigned long noShockDuration = 3000; // 5 seconds of no shock
const unsigned long resetDelay = 2000;      // Delay after sending message (3 seconds)
bool isTimerRunning = false;        // Flag to check if the timer is running

void setup() {
  Serial.begin(115200);
  
  // Set up flame sensor pin as input
  pinMode(FLAME_SENSOR_PIN, INPUT);

    // Initialize sensor pin
  pinMode(VIBRATION_SENSOR_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially

  // Wi-Fi connection
  Serial.println("Starting Wi-Fi connection...");
  WiFi.begin(WIFI_SSID);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 15000) {
      Serial.println("Failed to connect to Wi-Fi.");
      Serial.println("Wi-Fi Status: " + String(WiFi.status()));
      break;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());

    config.database_url = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    if (!Firebase.ready()) {
      Serial.println("Failed to connect to Firebase.");
      Serial.println(firebaseData.errorReason());
      return;
    }
    Serial.println("Connected to Firebase");
  } else {
    Serial.println("Wi-Fi connection failed, operating in offline mode.");
  }
}
void buzzAlert() {
  digitalWrite(BUZZER_PIN, HIGH); // Turn the buzzer on
  delay(10000);                   // Buzz for 10 seconds
  digitalWrite(BUZZER_PIN, LOW);  // Turn the buzzer off
}

void Flame(){
  // Read the flame sensor state
  int flameState = digitalRead(FLAME_SENSOR_PIN);
  Serial.print(flameState);

  String Flame;
  if (flameState == LOW) {
    Flame = "Flame Detected";  // Flame detected (sensor sends LOW when flame is detected)
  } else {
    Flame = "No Flame Detected";  // No flame detected
  }

  // Send Flame to Firebase only if there's a state change
  if (Flame != F_lastState) {
    if (Firebase.setString(firebaseData, "/sensors/FireStatus", Flame)) {
      Serial.print(" Flame sent: ");
      Serial.println(Flame);
      if (Flame == "Flame Detected") {
        buzzAlert();  // Trigger buzzer alert
      }
    } else {
      Serial.print(" Failed to send Flame: ");
      Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
    }
    F_lastState = Flame;  // Update last state to the current state
  }

  delay(100);  // Check every 2sec
}

void smoke(){
  // Read the analog value from the MQ2 sensor
  int smokeLevel = analogRead(MQ2_ANALOG_PIN);

  // Retrieve the smoke threshold value from Firebase
  if (Firebase.getInt(firebaseData, "/sensors/SmokeThreshold")) {
    if (firebaseData.dataType() == "int") {
      SMOKE_THRESHOLD = firebaseData.intData();  // Update threshold from Firebase
      Serial.println("Smoke Threshold: " + String(SMOKE_THRESHOLD));
    } else {
      Serial.println(SMOKE_THRESHOLD);
    }
  } else {
    Serial.println("Failed to get smoke threshold from Firebase");
  }

  // Prepare smoke status based on analog reading
  String smokeStatus;
  if (smokeLevel > SMOKE_THRESHOLD) {
    smokeStatus = "Smoke Detected";
  } else {
    smokeStatus = "No Smoke Detected";
  }

  // Update Firebase with smoke status
  if (Firebase.setString(firebaseData, "/sensors/SmokeStatus", smokeStatus)) {
    Serial.println("Smoke status updated successfully: " + smokeStatus);
    if (smokeStatus == "Smoke Detected") {
      buzzAlert();  // Trigger buzzer alert
    }
  } else {
    Serial.println("Failed to update smoke status");
    Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
  }

  // Update Firebase with smoke levels
  if (Firebase.setInt(firebaseData, "/sensors/SmokeLevel", smokeLevel)) {
    Serial.println("Smoke level updated successfully: " + String(smokeLevel));
  } else {
    Serial.println("Failed to update smoke levels");
    Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
  }

  delay(100);  // Wait 1 second before reading again
}


void shock() {
  int shockState = digitalRead(VIBRATION_SENSOR_PIN);
  Serial.print(shockState);

  // If shock is detected, start or continue the timer
  if (shockState == 1) {
    if (!isTimerRunning) {
      timerStart = millis();  // Start the timer on the first shock detection
      isTimerRunning = true;
      Serial.println("Shock detected - Starting timer.");
    } else {
      lastShockTime = millis();  // Update the last shock time on continuous shock detection
    }
  }

  // If timer is running, check for shock or no shock conditions
  if (isTimerRunning) {
    unsigned long currentTime = millis();

    // If shock is detected, reset the no-shock timer
    if (shockState == 1) {
      lastShockTime = currentTime;  // Update last shock time on detection
    }

    // If no shock for the defined period (5 seconds), reset the timer
    if (currentTime - lastShockTime >= noShockDuration) {
      Serial.println(" No shock for 5 seconds. Resetting timer.");
      isTimerRunning = false;
      delay(resetDelay);  // Pause for 3 seconds before resuming monitoring
    }

    // If timer reaches 10 seconds and shock is still detected, send "Shock Detected"
    else if (currentTime - timerStart >= timerDuration) {
      if (lastState != "Shock Detected") {
        if (Firebase.setString(firebaseData, "/sensors/shockStatus", "
         Shock detected")) {
          Serial.println("Shock detected - Message sent to Firebase.");
          buzzAlert();
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
      if (Firebase.setString(firebaseData, "/sensors/shockStatus", "NO Shock detected")) {
        Serial.println("NO Shock detected - Reset message sent to Firebase.");
      } else {
        Serial.print("Failed to send message: ");
        Serial.println(firebaseData.errorReason());
      }
      lastState = "No Shock Detected";
    }
  } else {
    // If no shock detected, reset the state
    if (Firebase.setString(firebaseData, "/sensors/shockStatus", "NO Shock detected")) {
      Serial.println(" NO shock detected - Message sent to Firebase.");
    } else {
      Serial.print("Failed to send message: ");
      Serial.println(firebaseData.errorReason());
    }
  }

  delay(100); // Stabilization delay
}

void loop() {
  Flame();
  smoke();
  shock();
}
