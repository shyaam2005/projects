// Bhavan and Akhileash project
#include <WiFi.h>
#include <Wire.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>  // Changed from Adafruit_BME280.h to Adafruit_BMP280.h
#include <DHT.h>

#define SEALEVEL_HPA (1013.25)
#define DHT_SENSOR_PIN 4
#define DHT_SENSOR_TYPE DHT11

#define soilMoisture  34
#define threshold  25
#define relay 15
#define ldr 33

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
Adafruit_BMP280 bmp;  // Changed from Adafruit_BME280 bme to Adafruit_BMP280 bmp

unsigned long delaytime;

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Akhileash's Galaxy S20 FE 5G" // WiFi name
#define WIFI_PASSWORD "iszs8637" // WiFi password

#define API_KEY "AIzaSyDxu3omBJ2A5X8D2UWXwNxwzJeema28VrE" // Web API key of Firebase

#define DATABASE_URL "https://crop-4351e-default-rtdb.asia-southeast1.firebasedatabase.app/" // URL of the RTDB Firebase



FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0; // Used for delay
unsigned int count = 0;
bool signupOK = false; // Identification of signing up                      

void setup() {
  Serial.begin(115200);
  pinMode(soilMoisture, INPUT);
  pinMode(relay, OUTPUT);
  pinMode(ldr,INPUT);

  //digitalWrite(relay, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP()); // Local IP address of ESP
  Serial.println();

  bool status;
  status = bmp.begin(0x76); // Initialize BMP280 sensor
  if (!status) {
    Serial.println("Could not find BMP value");
    while (1);
  }
  Serial.println("--Default test--");
  delaytime = 1000;

  Serial.println();

  /* Assign the API key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) { // Two empty brackets denotes anonymous users
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str()); // Tell what error happened in serial monitor
  }

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();
  float humidity = dht_sensor.readHumidity();
  float soil = analogRead(soilMoisture);
  float light = digitalRead(ldr);

  int moisture_level = map(soil, 0, 4095, 0, 100);

   // Soil moisture from soil moisture sensor
    if (moisture_level < threshold) {
      Serial.println("Moisture level is low");
      digitalWrite(relay, HIGH);
      delay(1000);
    }
    else if (moisture_level > (threshold+10)) {
      digitalWrite(relay,LOW);
      Serial.println("Moisture level is too high");
      delay(1000);
    }


    if (light == 1) {
      Serial.println("It is cloudy");
      delay(1000);
    }
    else {
      Serial.println("It is sunny");
      delay(1000);
    }


  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    // Since we want the data to be updated every second
    sendDataPrevMillis = millis();
    // Temperature from BMP280
    if (Firebase.RTDB.setFloat(&fbdo, "CIT/Temperature", temperature)) { // Sending float value
      Serial.print("Temperature : ");
      Serial.println(String(temperature) + " °C");
    } else {
      Serial.println("Failed to Read from the Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Pressure from BMP280
    if (Firebase.RTDB.setFloat(&fbdo, "CIT/Pressure", pressure)) { // Sending float value
      Serial.print("Pressure : ");
      Serial.println(String(pressure/1000) + " kPa");
    } else {
      Serial.println("Failed to Read from the Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Humidity from DHT11
    if (Firebase.RTDB.setInt(&fbdo, "CIT/Humidity", humidity)) { // Sending float value
      Serial.print("Humidity : ");
      Serial.println(String(int(humidity)) + "%");
    } else {
      Serial.println("Failed to Read from the Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //Soil moisture 
    if (Firebase.RTDB.setInt(&fbdo, "CIT/SoilMoistureLevel", moisture_level)) {
      Serial.print("Soil Moisture : ");
      Serial.println(moisture_level);
    } else {
      Serial.println("Failed to read from Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //LDR 
    if (Firebase.RTDB.setInt(&fbdo, "CIT/LightIntensity", light)) {
      Serial.print("Light Intensity : ");
      Serial.println(light);
    } else {
      Serial.println("Failed to read from Sensor");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}
