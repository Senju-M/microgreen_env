#include "DHT.h"

#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseFS.h>
#include <Firebase_ESP_Client.h>
#include <Arduino.h>
#include <WiFi.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "change_me"
#define WIFI_PASSWORD "change_me"

// Insert Firebase project API Key
#define API_KEY "change_me"

// Insert RTDB URL
#define DATABASE_URL "change_me"

#define RELAY_11 13
#define RELAY_12 14
#define MOISTURE_1 34

const int DHT11_1 = 2;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

DHT dht1(DHT11_1, DHT11);
DHT dht2(DHT11_2, DHT11);

unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  dht1.begin();
  pinMode(MOISTURE_1, INPUT);
  pinMode(RELAY_11, OUTPUT);
  pinMode(RELAY_12, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  int humi1 = dht1.readHumidity();
  int temp1 = dht1.readTemperature();
  int humiAuto1 = 0;
  int tempAuto1 = 0;
  int value1 = analogRead(MOISTURE_1);
  int moisture1 = 100.0 - ((value1 / 4095.0) * 100.0);
  int moistureAuto1 = 0;
  bool isAuto1;

  if (Firebase.RTDB.getBool(&fbdo, "sys1/auto")) {
    isAuto1 = fbdo.boolData();  // Store retrieved string in the variable
  } else {
    Serial.println("Failed to read auto 1 from Firebase");
    Serial.println(fbdo.errorReason());
  }
  if (isAuto1 == false) {
    Serial.println("Sys1 auto OFF");
    if (moisture1 > 20) {
      digitalWrite(RELAY_11, HIGH);
    } else if (moisture1 < 20) {
      digitalWrite(RELAY_11, LOW);
    }
    if (humi1 < 65) {
      digitalWrite(RELAY_12, HIGH);
    } else if (humi1 > 65) {
      digitalWrite(RELAY_12, LOW);
    }
    // Write humidity
    if (Firebase.RTDB.setInt(&fbdo, "sys1/humidity", humi1)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.print(humi1);
      Serial.printf("\n");
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Write temperature
    if (Firebase.RTDB.setInt(&fbdo, "sys1/temperature", temp1)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.print(temp1);
      Serial.printf("\n");
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Write moisture
    if (Firebase.RTDB.setInt(&fbdo, "sys1/moisture", moisture1)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.print(moisture1);
      Serial.printf("\n");
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  } else if (isAuto1 == true) {
    if (Firebase.RTDB.getInt(&fbdo, "prediction/humidity")) {
      humiAuto1 = fbdo.intData();
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "prediction/temperature")) {
      tempAuto1 = fbdo.intData();
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Write moisture
    if (Firebase.RTDB.getInt(&fbdo, "prediction/moisture")) {
      moistureAuto1 = fbdo.intData();
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (humi1 != humiAuto1) {
      digitalWrite(RELAY_12, HIGH);
    } else {
      digitalWrite(RELAY_12, LOW);
    }
    if (moisture1 != moistureAuto1) {
      digitalWrite(RELAY_11, HIGH);
    } else {
      digitalWrite(RELAY_11, LOW);
    }
  } else {
    Serial.println("auto 1 junk value");
  }
  delay(1000);
}