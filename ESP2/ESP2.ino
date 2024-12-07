#undef HIGH  // Undefine HIGH to prevent conflict
#include <Wire.h>
#include <Keypad.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

// DHT11 Sensor Pin Definitions
#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// I2C Addresses
#define PCF8574_ADDR 0x20
#define LCD_ADDR 0x27

// Define I2C Pins for ESP8266
#define SDA_PIN D2
#define SCL_PIN D1 

bool passwordMode = true;  // Start in password mode




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
  void pin_mode(byte pinNum, byte mode) override {}
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
#define FIREBASE_HOST "https://smartlock-46110-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "meqpOYjcPQLL2CtuiFIgRUM4YQjU6KJSC4zAqs7E"

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth; // Empty auth object
FirebaseConfig config;

// Variables for storing user input
String enteredPin = "";
String correctPin = "";  // Will be fetched from Firebase

// Function to initialize PCF8574
void initializePCF8574() {
  if (pcf8574.begin()) {
    Serial.println("PCF8574 Initialized!");
  } else {
    Serial.println("Error initializing PCF8574!");
    while (1);  // Halt if there's an error
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  

  // Initialize I2C for ESP8266
  Wire.begin(SDA_PIN, SCL_PIN);


  // Initialize PCF8574
  initializePCF8574();

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

  // Display message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter PIN:");
}

void checkTemperature() {
  // Delay between measurements
  delay(100);

  // Read temperature from DHT11
  float temperature = dht.readTemperature();

  // Check if readings are valid
  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Define static temperature thresholds
  String status;
  if (temperature >= 30 && temperature < 50) {
    status = "NORMAL";
  } else if (temperature >= 60 && temperature < 70) {
    status = "WARNING";
  } else if (temperature >= 80 && temperature <= 100) {
    status = "OVERHEATING";
  } else {
    status = "UNKNOWN";
  }

  // Log temperature and status

  
  Serial.println("Temperature: " + String(temperature) + "°C, Status: " + status);

  // Update Firebase with temperature and status
  if (Firebase.setFloat(firebaseData, "/sensors/Temperature", temperature)) {
    Serial.println("Temperature updated successfully.");
  } else {
    Serial.println("Failed to update temperature.");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setString(firebaseData, "/sensors/MCUstatus", status)) {
    Serial.println("Status updated successfully: " + status);
  } else {
    Serial.println("Failed to update status.");
    Serial.println(firebaseData.errorReason());
  }
}


void checkRestart() {
  Firebase.getInt(firebaseData, "/sensors/Restart");
  int restartFlag = firebaseData.intData();

  if (restartFlag == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Restarting Hardware...");
    delay(2000);
    ESP.restart();
  } else if (restartFlag == 1) {
    PASSWORD();
  }
}

void PASSWORD() {
  char customKey = customKeypad.getKey();  // Check if a key is pressed
  
  if (customKey) {
    enteredPin += customKey;  // Add the pressed key to the entered PIN

    // Display the entered key as an asterisk
    lcd.setCursor(enteredPin.length() - 1, 1);
    lcd.print("*");

    // If the PIN length reaches 6 characters, check it
    if (enteredPin.length() == 6) {
      Serial.println("Entered PIN: " + enteredPin);
      lcd.clear();

      // Check if the entered PIN matches the correct PIN
      if (enteredPin == correctPin) {
        lcd.setCursor(0, 0);
        lcd.print("PIN Correct");
        Firebase.setInt(firebaseData, "/sensors/LockStatus", 0);  // Unlock the lock
        Serial.println("Lock unlocked");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("PIN Incorrect");
        Firebase.setInt(firebaseData, "/sensors/LockStatus", 1);  // Lock the system
        Serial.println("Lock locked");
      }

      // Reset PIN entry
      enteredPin = "";
      delay(2000);  // Short delay to display the result before clearing the screen
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter PIN:");
    }
  }
}


void loop() {

  checkRestart();
  checkTemperature();

}
