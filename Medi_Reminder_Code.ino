#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

RTC_DS3231 rtc; // Create an RTC object
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address and the number of columns and rows
Servo myservo;

const int ledPin = 13; // LED connected to digital pin 13
const int buzzerPin = 12; // Buzzer connected to digital pin 12

bool alarmActivated = false;
int alarmHour = 0;
int alarmMinute = 0;
unsigned long alarmStartTime = 0;
const unsigned long alarmDuration = 10000; // 10 seconds
const unsigned long motorDuration = 1000; // 1 second for the motor to rotate from 90 to 0 degrees
const unsigned long ledBlinkDuration = 5000; // 5 seconds for LED blinking

#define SIM800_TX 7 // Connect this to the TX pin of your SIM800 module
#define SIM800_RX 8 // Connect this to the RX pin of your SIM800 module
SoftwareSerial sim800(SIM800_TX, SIM800_RX);

void setup() {
  Serial.begin(9600);
  sim800.begin(9600); // Set the baud rate to match your SIM800 module

  Wire.begin(); // Initialize I2C communication
  rtc.begin(); // Initialize the RTC
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as an output
  myservo.attach(9);
  myservo.write(0); // Initialize the servo at 0 degrees

  lcd.setCursor(0, 0);
  lcd.print("HELLO");
  lcd.setCursor(0, 1);
  lcd.print("DISPENSER !!");

  delay(2000); // Display message for 3 seconds and then clear the LCD
  lcd.clear();

  lcd.print("Current Time:");
  lcd.setCursor(0, 1);
  lcd.print(formatTime(rtc.now().hour(), rtc.now().minute(), rtc.now().second()));
}

void loop() {
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();
  int currentSecond = now.second();

  lcd.setCursor(0, 1);
  lcd.print(formatTime(currentHour, currentMinute, currentSecond));

  if (alarmActivated) {
    if (currentHour == alarmHour && currentMinute == alarmMinute) {
      unsigned long motorStart = millis(); // Record the start time for the motor movement
      unsigned long halfDuration= 500 / 2;
      while (millis() - motorStart <= motorDuration) {
        if (millis() - motorStart <= halfDuration ) {
          myservo.write(90); // Set the servo angle to 90 degrees
        } else {
          myservo.write(0); // Return the servo to 0 degrees
        }
      }

      // LED blinking for 5 seconds
      for (unsigned long ledStartTime = millis(); millis() - ledStartTime < ledBlinkDuration;) {
        digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle the LED state
        delay(250); // Blink every 250ms
      }

      digitalWrite(ledPin, HIGH); // Keep the LED on after blinking
      alarmBuzzer(); // Activate the buzzer
      sendSMS("+917418738863", "Your Medicine Has Dispensed!"); // Send an SMS
      displayMessage("Medicine Is Dispensed :)"); // Display message
      delay(3000); // Display message for 3 seconds
      lcd.clear(); // Clear the LCD
      digitalWrite(ledPin, LOW);
      alarmActivated = false;
      lcd.setCursor(0, 0);
      lcd.print("Current Time:");
      lcd.setCursor(0, 1);
      lcd.print(formatTime(currentHour, currentMinute, currentSecond));
    }
  }

  if (Serial.available() > 0) {
    String input = Serial.readString();
    if (input.startsWith("SET ")) {
      input.remove(0, 4); // Remove the "SET " prefix
      int newHour = input.substring(0, 2).toInt();
      int newMinute = input.substring(3, 5).toInt();
      if (newHour >= 0 && newHour <= 23 && newMinute >= 0 && newMinute <= 59) {
        alarmHour = newHour;
        alarmMinute = newMinute;
        alarmActivated = true;
        Serial.println("Medicine Dispensing Time Set.");
      } else {
        Serial.println("Invalid time format. Please enter a valid time (HH:MM).");
      }
    }
  }
}

String formatTime(int hour, int minute, int second) {
  return String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second);
}

void sendSMS(const char *phoneNumber, const char *message) {
  Serial.println(F("Sending SMS...Message Sent"));
  sim800.print(F("AT+CMGF=1\r")); // Set SMS mode to text
  delay(1000);
  sim800.print(F("AT+CMGS=\""));
  sim800.print(phoneNumber);
  sim800.println(F("\""));
  delay(1000);
  sim800.print(message);
  sim800.write(0x1A); // Send Ctrl+Z to indicate the end of the message
  delay(1000);
}

void displayMessage(const char *message) {
  lcd.clear(); // Clear the LCD
  lcd.setCursor(0, 0);
  lcd.print("Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Is Dispensed :)");
}

void alarmBuzzer() {
  int buzzerIntensity = 0;
  while (buzzerIntensity < 255) {
    analogWrite(buzzerPin, buzzerIntensity);
    buzzerIntensity += 3; // Adjust this step value for the desired fade speed
    delay(50); // Adjust this delay for the desired fade speed
  }
  
  delay(500); // Buzzer active at full intensity for 1 second
  
  while (buzzerIntensity > 0) {
    analogWrite(buzzerPin, buzzerIntensity);
    buzzerIntensity -= 3; // Adjust this step value for the desired fade speed
    delay(50); // Adjust this delay for the desired fade speed
  }
  analogWrite(buzzerPin, 0); // Turn off the buzzer
}
