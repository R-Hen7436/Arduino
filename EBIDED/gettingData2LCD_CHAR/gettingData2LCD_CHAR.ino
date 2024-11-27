#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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

String previousMessage = "";  // To store the previous string from Firebase

// Set up the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16 columns and 2 rows

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

  // Initialize the LCD
  lcd.begin();
  lcd.backlight(); // Turn on the backlight

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

  // Display initial message on LCD
  lcd.setCursor(0, 0);
  lcd.print("Connected to Firebase");
  delay(2000);  // Wait for 2 seconds to show the message
}

void loop() {
  // Read the value from Firebase (for test/message path)
  if (Firebase.getString(firebaseData, "/test/message")) {
    String currentMessage = firebaseData.stringData(); // Get the string value

    // Only print if the value has changed
    if (currentMessage != previousMessage) {
      Serial.print("Firebase message: ");
      Serial.println(currentMessage);

      // Display the value on the LCD
      lcd.clear(); // Clear the screen
      lcd.setCursor(0, 0); // Move cursor to first row
      lcd.print("Message: ");  // Display label
      lcd.setCursor(0, 1);  // Move cursor to second row
      lcd.print(currentMessage); // Display the message

      previousMessage = currentMessage;  // Update the previous message
    }
  } else {
    Serial.println("Failed to read message");
    Serial.println(firebaseData.errorReason()); // Print the error reason for the failed read
  }

  delay(1000);  // Delay to reduce the number of reads (adjust this as needed)
}
