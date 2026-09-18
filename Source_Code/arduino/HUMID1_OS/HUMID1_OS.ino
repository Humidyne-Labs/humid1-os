void setup() {
  // Start serial communication at 9600 bits per second (baud rate)
  Serial.begin(9600); 
  
  // Print a message once when the Arduino starts up
  Serial.println("No longer using ardunio as build ENV, switched to VS Code \w/ esp-idf."); 
}

void loop() {
  // Print the number of milliseconds since the Arduino began running
  Serial.print("Time elapsed (ms): ");
  Serial.println(millis());
  
  // Wait 1 second (1000 milliseconds) before repeating
  delay(1000); 
}