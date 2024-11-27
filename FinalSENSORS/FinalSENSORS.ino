#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

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

// DHT11 Sensor Pin Definitions
#define DHTPIN D4     // Pin where the DHT11 is connected
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

#define MQ2_DIGITAL_PIN D3    // Digital output pin for the MQ2 smoke sensor
#define MQ2_ANALOG_PIN A0     // Analog output pin for the MQ2 smoke sensor
#define FLAME_SENSOR_PIN D1   // Pin connected to the flame sensor

// Smoke detection threshold
const int SMOKE_THRESHOLD = 300; // Example threshold for smoke concentration

int previousValue = -1;  // To store the previous value from Firebase
String lastFlameState = "";  // To store the last detected flame state

void setup() {
  Serial.begin(115200);
  dht.begin(); // Start the DHT sensor

  // Initialize, and MQ2_SENSOR pins
  pinMode(MQ2_DIGITAL_PIN, INPUT);   // Set smoke sensor digital pin as input
  pinMode(FLAME_SENSOR_PIN, INPUT);  // Set flame sensor pin as input

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
  int flameState = digitalRead(FLAME_SENSOR_PIN);
  String flameStatus = (flameState == LOW) ? "Flame Detected" : "No Flame Detected";

  // Update Firebase with flame status only if it changed
  if (flameStatus != lastFlameState) {
    if (Firebase.setString(firebaseData, "/users/Alren/sensors/Flame", flameStatus)) {
      Serial.print("Flame status updated: ");
      Serial.println(flameStatus);
    } else {
      Serial.println("Failed to update flame status");
      Serial.println(firebaseData.errorReason());
    }
    lastFlameState = flameStatus;  // Update last state to the current state
  }
}

void checkSmokeStatus() {
  int smokeDetected = digitalRead(MQ2_DIGITAL_PIN);  // Read digital output for smoke detection
  int smokeLevel = analogRead(MQ2_ANALOG_PIN);       // Read analog output for smoke level

  // Determine smoke status based on digital reading
  String smokeStatus = (smokeDetected == LOW) ? "Smoke Detected" : "No Smoke Detected";

  // Update Firebase with smoke status
  if (Firebase.setString(firebaseData, "/users/Alren/sensors/Smoke", smokeStatus)) {
    Serial.println("Smoke status updated successfully: " + smokeStatus);
  } else {
    Serial.println("Failed to update smoke status");
    Serial.println(firebaseData.errorReason());
  }

  // Update Firebase with smoke level
  if (Firebase.setInt(firebaseData, "/users/Alren/sensors/SmokeLevel", smokeLevel)) {
    Serial.println("Smoke level updated successfully: " + String(smokeLevel));
  } else {
    Serial.println("Failed to update smoke level");
    Serial.println(firebaseData.errorReason());
  }
}

void checkTemperatureAndHumidity() {
  float humidity = dht.readHumidity();          // Read humidity (percent)
  float temperature = dht.readTemperature();    // Read temperature in Celsius

  // Check if readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return; // Exit if there was an error
  }

  // Print to Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  // Update Firebase with temperature and humidity
  if (Firebase.setFloat(firebaseData, "/users/Alren/sensors/Temperature", temperature)) {
    Serial.println("Temperature updated successfully: " + String(temperature));
  } else {
    Serial.println("Failed to update temperature");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setFloat(firebaseData, "/users/Alren/sensors/Humidity", humidity)) {
    Serial.println("Humidity updated successfully: " + String(humidity));
  } else {
    Serial.println("Failed to update humidity");
    Serial.println(firebaseData.errorReason());
  }
}

void loop() {

  // Check smoke status
  checkSmokeStatus();

  // Check flame status
  checkFlameStatus();

  // Check temperature and humidity
  checkTemperatureAndHumidity();

  delay(2000);  // Delay to reduce the number of reads (adjust as needed)
}
