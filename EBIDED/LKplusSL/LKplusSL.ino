#undef HIGH  // Undefine HIGH to prevent conflict
#include <Wire.h>
#include <Keypad.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// I2C Addresses
#define PCF8574_ADDR 0x20
#define LCD_ADDR 0x27

// Define I2C Pins for ESP8266
#define SDA_PIN D2
#define SCL_PIN D1

// Initialize PCF8574
PCF8574 pcf8574(PCF8574_ADDR);

// Initialize LCD
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);  // 16x2 LCD

// Define the keypad size
const byte ROWS = 4; 
const byte COLS = 4; 

// Define the keypad layout
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'}, 
  {'*', '0', '#', 'D'}
};

// Define PCF8574 pins for rows and columns
byte rowPins[ROWS] = {0, 1, 2, 3};  // PCF8574 pins P0-P3
byte colPins[COLS] = {4, 5, 6, 7};  // PCF8574 pins P4-P7

// Keypad pin emulation using PCF8574
class KeypadPCF8574 : public Keypad {
public:
  KeypadPCF8574(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols) 
    : Keypad(userKeymap, row, col, numRows, numCols) {}

protected:
  void pin_mode(byte pinNum, byte mode) override {
    // PCF8574 library doesn't require explicit pin mode setup
  }
  void pin_write(byte pinNum, bool level) override {
    pcf8574.write(pinNum, level);
  }
  int pin_read(byte pinNum) override {
    return pcf8574.read(pinNum);
  }
};

// Instantiate KeypadPCF8574 object
KeypadPCF8574 customKeypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Firebase configuration
#define WIFI_SSID "UbasC"
#define FIREBASE_HOST "https://smartlock-46110-default-rtdb.asia-southeast1.firebasedatabase.app" // Full URL
#define FIREBASE_AUTH "meqpOYjcPQLL2CtuiFIgRUM4YQjU6KJSC4zAqs7E" // Firebase secret token

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; // Empty auth object
FirebaseConfig config;

// Pin connected to the solenoid lock (Relay module)
//#define SOLENOID_PIN D5  // Use GPIO pin D5 (adjust as needed)

// Variables for storing user input
String enteredPin = "";
String correctPin = "";  // Will be fetched from Firebase

void setup() {
  Serial.begin(115200);

  // Initialize I2C for ESP8266
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize PCF8574
  if (pcf8574.begin()) {
    Serial.println("PCF8574 Initialized!");
  } else {
    Serial.println("Error initializing PCF8574!");
    while (1);  // Halt if there's an error
  }

  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to Wi-Fi...");

  // Start Wi-Fi connection
  WiFi.begin(WIFI_SSID);
  
  // Wi-Fi connection timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 15000) {  // 15-second timeout
      Serial.println("Failed to connect to Wi-Fi.");
      return;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Firebase configuration
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase with config and empty auth object
  Firebase.begin(&config, &auth); 
  Firebase.reconnectWiFi(true);

  // Fetch correct PIN from Firebase
  Firebase.getString(firebaseData, "/sensors/LockPassword");
  correctPin = firebaseData.stringData();
  
  // Initialize solenoid pin
  //pinMode(SOLENOID_PIN, OUTPUT);
  //digitalWrite(SOLENOID_PIN, LOW);  // Keep the lock initially in the locked position (LOW)

  // Display message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter PIN:");
}

void loop() {
  char customKey = customKeypad.getKey();
  
  if (customKey) {
    // Append the entered key to the PIN
    enteredPin += customKey;

    // Display the entered key on the LCD
    lcd.setCursor(enteredPin.length() - 1, 1);  // Display at the corresponding position
    lcd.print("*");  // Show asterisks for security

    // Check if 6 digits have been entered
    if (enteredPin.length() == 6) {
      delay(500);  // Small delay for readability

      // Compare entered PIN with the correct PIN
      lcd.clear();
      if (enteredPin == correctPin) {
        lcd.setCursor(0, 0);
        lcd.print("PIN Correct");
        
        // Unlock solenoid
        //digitalWrite(SOLENOID_PIN, HIGH);  // Unlock the solenoid lock
        Firebase.setInt(firebaseData, "/sensors/LockStatus", 0);  // Update Firebase to reflect unlock status
        Serial.println("Lock unlocked");

      } else {
        lcd.setCursor(0, 0);
        lcd.print("PIN Incorrect");
        
        // Lock solenoid
        //digitalWrite(SOLENOID_PIN, LOW);  // Lock the solenoid lock
        Firebase.setInt(firebaseData, "/sensors/LockStatus", 1);  // Update Firebase to reflect lock status
        Serial.println("Lock locked");
      }

      // Clear entered PIN after checking
      enteredPin = "";

      // Reset the prompt
      delay(2000);  // Display result for 2 seconds
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter PIN:");
    }
  }
}