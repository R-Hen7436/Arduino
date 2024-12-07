#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Wi-Fi credentials
#define WIFI_SSID "UbasC"
// #define WIFI_PASSWORD "C@bigonUb#s**2023**" // Use for Wi-Fi password if required

// Firebase credentials
#define FIREBASE_HOST "https://smartlock-46110-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "meqpOYjcPQLL2CtuiFIgRUM4YQjU6KJSC4zAqs7E"

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; 
FirebaseConfig config;



// Solenoid Lock Pin
#define SOLENOID_PIN D5  

// Touch Sensor Pin
#define TOUCH_SENSOR_PIN D8  


int previousValue = -1;


void setup() {
  Serial.begin(115200);


  // Initialize sensor and LED pins
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);  
  pinMode(TOUCH_SENSOR_PIN, INPUT);

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

void loop() {


  if (WiFi.status() == WL_CONNECTED) {
    if (Firebase.getInt(firebaseData, "/sensors/LockStatus")) {
      int currentValue = firebaseData.intData();

      if (currentValue != previousValue) {
        Serial.print("Firebase value: ");
        Serial.println(currentValue);

        if (currentValue == 1) {
          digitalWrite(SOLENOID_PIN, HIGH);
          Serial.println("LOCK LOCKED");
        } else if (currentValue == 0) {
          digitalWrite(SOLENOID_PIN, LOW);
          Serial.println("LOCK UNLOCKED");
        }

        previousValue = currentValue;
      }
    } else {
      Serial.println("Failed to read value from Firebase");
      Serial.println(firebaseData.errorReason());
    }
  } else {
    if (digitalRead(TOUCH_SENSOR_PIN) == HIGH) {
      Serial.println("Touch detected. Toggling lock state.");
      int currentLockState = digitalRead(SOLENOID_PIN);
      if (currentLockState == HIGH) {
        digitalWrite(SOLENOID_PIN, LOW);
        Serial.println("LOCK LOCKED");
        delay(3000);
      } else {
        digitalWrite(SOLENOID_PIN, HIGH);
        Serial.println("LOCK UNLOCKED");
        delay(3000);
      }
      delay(2000);
    }
  }

}
