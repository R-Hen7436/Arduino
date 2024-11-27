#include <Wire.h>
#include <Keypad.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>

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

// Predefined correct PIN
const String correctPin = "#1223D";

// Variables for storing user input
String enteredPin = "";

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
      } else {
        lcd.setCursor(0, 0);
        lcd.print("PIN Incorrect");
      }

      Serial.println(enteredPin == correctPin ? "PIN Correct" : "PIN Incorrect");

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
