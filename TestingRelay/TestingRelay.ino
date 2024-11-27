void setup() {
  Serial.begin(115200);
  pinMode(D0, OUTPUT);
}

void loop() {
  digitalWrite(D0, HIGH);  // Turn relay ON
  Serial.println("Relay ON (HIGH)");
  delay(2000);              // Keep it on for 2 seconds
  
  digitalWrite(D0, LOW);   // Turn relay OFF
  Serial.println("Relay OFF (LOW)");
  delay(2000);              // Keep it off for 2 seconds
}
