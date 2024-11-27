#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

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

// DHT11 Sensor Pin Definitions
#define DHTPIN D2     // Pin where the DHT11 is connected
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud
  dht.begin();          // Start the DHT sensor

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
  // Wait a few seconds between measurements
  delay(2000);

  // Read humidity and temperature from the DHT11
  float humidity = dht.readHumidity();          // Read humidity (percent)
  float temperature = dht.readTemperature();    // Read temperature in Celsius

  // Check if readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return; // Exit loop if there was an error
  }

  // Print the results to the Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  // Update Firebase with temperature and humidity
  if (Firebase.setFloat(firebaseData, "/test/Temperature", temperature)) {
     Serial.println("----------------------------------------------" );
    Serial.println("Temperature updated successfully: " + String(temperature));
  } else {
    Serial.println("Failed to update temperature");
    Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
  }

  if (Firebase.setFloat(firebaseData, "/test/Humidity", humidity)) {
    Serial.println("----------------------------------------------" );
    Serial.println("Humidity updated successfully: " + String(humidity));
  } else {
    Serial.println("Failed to update humidity");
    Serial.println(firebaseData.errorReason()); // Print error reason for failed data send
  }
   Serial.println("----------------------------------------------" );
}
