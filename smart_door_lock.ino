

/*
ESP32S3 DEV MODUAL
-----------------------------------------------------------------------------
* RST/Reset   RST          GPIO-14
* SPI SS      SDA(SS)      GPIO-5
* SPI MOSI    MOSI         GPIO-11 
* SPI MISO    MISO         GPIO-13
* SPI SCK     SCK          GPIO-12
*/
08#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>


/* ========= I2C ========= */
#define SDA_PIN 8
#define SCL_PIN 9
LiquidCrystal_I2C lcd(0x27, 16, 2);


/* ========= RFID (CONFIRMED PINOUT) ========= */
#define SS_PIN   5
#define RST_PIN  14
#define SCK_PIN  12
#define MOSI_PIN 11
#define MISO_PIN 13
MFRC522 rfid(SS_PIN, RST_PIN);


/* ========= RGB ========= */
#define RGB_PIN 48
Adafruit_NeoPixel rgb(1, RGB_PIN, NEO_GRB + NEO_KHZ800);


/* ========= BUZZER ========= */
#define BUZZER_PIN 21


/* ========= AUTH CARD ========= */
const String AUTH_UID = "19CD348E";


/* ========= WIFI LIST ========= */
struct WiFiCred {
 const char* ssid;
 const char* pass;
};


WiFiCred wifiList[] = {
 {"Excitel_HARSHIT_2.4G", "dinesh480tak"},
 {"ESP-404",             "k756756756"},
 {"SKIT-WiFi 2.4G",      "skit@2024"}
};


const int WIFI_COUNT = sizeof(wifiList) / sizeof(wifiList[0]);
bool wifiConnected = false;


/* ========= VAR ========= */
bool otaRunning = false;
unsigned long lastCardTime = 0;


/* ========= HELPERS ========= */
void buzzerBeep(unsigned long durationMs) {
 digitalWrite(BUZZER_PIN, HIGH);
 delay(durationMs);
 digitalWrite(BUZZER_PIN, LOW);
}


void setLED(uint8_t r, uint8_t g, uint8_t b) {
 rgb.clear();
 rgb.setPixelColor(0, rgb.Color(r, g, b));
 rgb.show();
}


/* ========= SETUP ========= */
void setup() {
 Serial.begin(115200);
 delay(1000);


 Wire.begin(SDA_PIN, SCL_PIN);


 lcd.init();
 lcd.backlight();
 lcd.print("ESP32-S3 RFID");
 lcd.setCursor(0,1);
 lcd.print("Booting...");
 delay(1500);


 rgb.begin();
 rgb.clear();
 rgb.show();


 pinMode(BUZZER_PIN, OUTPUT);
 digitalWrite(BUZZER_PIN, LOW);


 /* ========= WIFI MULTI CONNECT ========= */
 WiFi.mode(WIFI_STA);
 WiFi.setSleep(false);


 lcd.clear();
 lcd.print("WiFi Searching");


 for (int i = 0; i < WIFI_COUNT && !wifiConnected; i++) {
   lcd.clear();
   lcd.print("Trying WiFi:");
   lcd.setCursor(0,1);
   lcd.print(wifiList[i].ssid);


   Serial.print("Trying WiFi: ");
   Serial.println(wifiList[i].ssid);


   WiFi.begin(wifiList[i].ssid, wifiList[i].pass);


   unsigned long startAttempt = millis();
   while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
     delay(300);
   }


   if (WiFi.status() == WL_CONNECTED) {
     wifiConnected = true;
     lcd.clear();
     lcd.print("WiFi Connected");
     lcd.setCursor(0,1);
     lcd.print(WiFi.localIP());


     Serial.println("WiFi Connected!");
     Serial.println(WiFi.localIP());
   } else {
     WiFi.disconnect(true);
     delay(500);
   }
 }


 if (!wifiConnected) {
   lcd.clear();
   lcd.print("WiFi FAILED");
   lcd.setCursor(0,1);
   lcd.print("Check Network");
 }


 delay(1500);


 /* ========= OTA ========= */
 ArduinoOTA.setHostname("ESP32-S3-RFID");


 ArduinoOTA.onStart([]() {
   otaRunning = true;
   lcd.clear();
   lcd.print("OTA Updating");
   setLED(0,0,255);
 });


 ArduinoOTA.onEnd([]() {
   otaRunning = false;
   lcd.clear();
   lcd.print("OTA Done");
   setLED(0,0,0);
   delay(1000);
 });


 ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
   lcd.setCursor(0,1);
   lcd.print((p * 100) / t);
   lcd.print("%   ");
 });


 ArduinoOTA.begin();


 /* ========= RFID ========= */
 SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
 rfid.PCD_Init();


 lcd.clear();
 lcd.print("RFID READY");
 lcd.setCursor(0,1);
 lcd.print("Tap Card");


 Serial.println("RFID System Ready");
}


/* ========= LOOP ========= */
void loop() {
 ArduinoOTA.handle();
 if (otaRunning) return;


 if (millis() - lastCardTime < 1200) return;
 if (!rfid.PICC_IsNewCardPresent()) return;
 if (!rfid.PICC_ReadCardSerial()) return;


 lastCardTime = millis();


 String uid = "";
 for (byte i = 0; i < rfid.uid.size; i++) {
   if (rfid.uid.uidByte[i] < 0x10) uid += "0";
   uid += String(rfid.uid.uidByte[i], HEX);
 }
 uid.toUpperCase();


 Serial.println("Card Detected: " + uid);


 lcd.clear();


 if (uid == AUTH_UID) {
   setLED(0,255,0);   // GREEN
   lcd.setCursor(0,0);
   lcd.print("CARD ACCEPTED");
   lcd.setCursor(0,1);
   lcd.print("CLASSROOM OPEN");
 } else {
   setLED(255,0,0);   // RED
   buzzerBeep(2000);
   lcd.setCursor(0,0);
   lcd.print("CARD NOT");
   lcd.setCursor(0,1);
   lcd.print("AUTHORIZED");
 }


 delay(1500);


 lcd.clear();
 lcd.setCursor(0,0);
 lcd.print("UID:");
 lcd.setCursor(0,1);
 lcd.print(uid);


 delay(1500);


 setLED(0,0,0);
 lcd.clear();
 lcd.print("RFID READY");
 lcd.setCursor(0,1);
 lcd.print("Tap Card");


 rfid.PICC_HaltA();
 rfid.PCD_StopCrypto1();
}



