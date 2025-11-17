#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include "MAX30105.h"
#include "heartRate.h"

#define MQ135   A0
#define ss      10
#define rst     9
#define dio0    2

MPU6050 mpu;
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
SoftwareSerial gpsSerial(4, 5);
TinyGPSPlus gps;
MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;

String LoRaMessage = "";
unsigned long prevLora = millis();
const long intervalResultHR = 1000; 
String msgLora;
DynamicJsonDocument doc(256);

int16_t ax_offset = 0, ay_offset = 0, az_offset = 0;
int16_t offsetX = 0;
int16_t offsetY = 0;

void calibrateCompass() {
  int16_t maxX = -32768, maxY = -32768;
  int16_t minX =  32767, minY =  32767;

  for (int i = 0; i < 500; i++) {
    sensors_event_t event;
    mag.getEvent(&event);

    int16_t x = event.magnetic.x;
    int16_t y = event.magnetic.y;

    if (x > maxX) maxX = x;
    if (x < minX) minX = x;
    if (y > maxY) maxY = y;
    if (y < minY) minY = y;

    delay(20); // biar sempat dibaca
  }

  offsetX = (maxX + minX) / 2;
  offsetY = (maxY + minY) / 2;
}

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Wire.begin();

  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 tidak terhubung!");
    while (1);
  }
  delay(1000);  
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  ax_offset = ax;
  ay_offset = ay;
  az_offset = az;

  if (!mag.begin()) {
    Serial.println("HMC5883L tidak terdeteksi!");
    while (1);
  }
  calibrateCompass();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 tidak ditemukan!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  while (!Serial);
    Serial.println("LoRa Sender");
    LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(433E6)) {
      Serial.println("Starting LoRa failed!");
      delay(100);
      while (1);
    }
}

void loop() {
  long irValue = particleSensor.getIR();
  
  if (irValue < 50000) {
    beatAvg = 0;
  } else {
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - prevLora >= intervalResultHR) {
    prevLora = currentMillis;

    // GAS
    int sensorValue = analogRead(MQ135);

    // GYRO
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    ax -= ax_offset;
    ay -= ay_offset;
    az -= az_offset;
    float ax_g = (float)ax / 16384.0;
    float ay_g = (float)ay / 16384.0;
    float az_g = (float)az / 16384.0;
    float totalAccel = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    bool shockDetected = totalAccel > 0.5;
    if (shockDetected) {
      Serial.println("!!! GUNCANGAN TERDETEKSI !!!");
    }
    
    // KOMPAS
    sensors_event_t event;
    mag.getEvent(&event);

    float x_corr = event.magnetic.x - offsetX;
    float y_corr = event.magnetic.y - offsetY;

    float heading = atan2(y_corr, x_corr);
    float declinationAngle = 0.22;
    heading += declinationAngle;
    if (heading < 0) heading += 2 * PI;
    if (heading > 2 * PI) heading -= 2 * PI;

    float headingDegrees = heading * 180 / M_PI;
    Serial.print("Heading (derajat): ");
    Serial.println(headingDegrees);


    // GPS
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }

    String gpsInfo = "Null";
    float lat = -0.9584123370029327;
    float lon = 100.3971301617781;
    if (gps.location.isValid()) {
      lat = gps.location.lat();
      lon = gps.location.lng();
      gpsInfo = "Lat: " + String(lat, 6) + " | Lon: " + String(lon, 6);
    } 

    Serial.println("BPM: " + String(beatAvg));
    Serial.println("Gas MQ135: " + String(sensorValue));
    Serial.println("MPU6050 - X: " + String(ax) + " Y: " + String(ay) + " Z: " + String(az));
    Serial.println("Heading Kompas: " + String(headingDegrees, 2) + "°");
    Serial.println("Lokasi : " + gpsInfo);

    String msgLora = String(beatAvg)+"~";
           msgLora+=String(sensorValue)+"~";
           msgLora+=String(ax)+"~";
           msgLora+=String(ay)+"~";
           msgLora+=String(az)+"~";
           msgLora+=String(headingDegrees)+"~";
           msgLora+=String(lat)+"~";
           msgLora+=String(lon)+"~";
           msgLora+=String(shockDetected ? 1 : 0);

    LoRa.beginPacket();
    LoRa.print(msgLora);
    LoRa.endPacket();
    //---------------
  }
}
