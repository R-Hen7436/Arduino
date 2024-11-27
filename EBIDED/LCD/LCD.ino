#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 (check your LCD module for the correct address)
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16 columns and 2 rows

void setup() {
  // Initialize the LCD
  lcd.begin();
  lcd.backlight(); // Turn on the backlight
  
  // Display "Hello" on the LCD
  lcd.setCursor(0, 0); // Set cursor to the first column and first row
  lcd.print("Hello TayinGwapo");
}

void loop() {
  // Nothing to do here
}
