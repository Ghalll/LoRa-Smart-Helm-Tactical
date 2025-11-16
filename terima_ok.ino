#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <U8g2lib.h>
#include <LoRa.h>
#include <ArduinoJson.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const char* ssid = "Hanss";
const char* password = "Hanzz1234";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "TesSkripsi";
DynamicJsonDocument doc(256);

WiFiClient espClient;
PubSubClient client(espClient);

#define LED_MERAH  12
#define LED_BIRU   13
#define LED_HIJAU  15
#define BUZZER     4
#define ss         5
#define rst        14
#define dio0       2

String myArray[9];

unsigned long lastLoRaDataTime = 0;
unsigned long lastBuzzerBlinkTime = 0;
unsigned long lastLED3BlinkTime = 0;
unsigned long led3BlinkStart = 0;
bool led3Blink = false;
bool buzzerState = false;

const unsigned long buzzerBlinkOn = 300;
const unsigned long buzzerBlinkOff = 700;
const unsigned long noLoRaTimeout = 5000;

void showOLED(String baris1 = "", String baris2 = "", String baris3 = "") {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  drawCenteredStr(12, baris1);
  drawCenteredStr(28, baris2);
  drawCenteredStr(44, baris3);
  u8g2.sendBuffer();
}

void drawCenteredStr(int y, String text) {
  int16_t x = (128 - u8g2.getStrWidth(text.c_str())) / 2;
  u8g2.drawStr(x, y, text.c_str());
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);

  int ledIndex = 0;
  int ledPins[] = {LED_MERAH, LED_BIRU, LED_HIJAU};
  int ledCount = sizeof(ledPins) / sizeof(ledPins[0]);
  int dotCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < ledCount; i++) {
      digitalWrite(ledPins[i], LOW);
    }
    digitalWrite(ledPins[ledIndex], HIGH);
    ledIndex = (ledIndex + 1) % ledCount;
    delay(300);
    String dots = "";
    for (int i = 0; i <= dotCount; i++) dots += ".";
    showOLED("Connecting: " + String(ssid), dots, "");
    dotCount = (dotCount + 1) % 4;
  }

  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(LED_BIRU, HIGH);
    digitalWrite(LED_HIJAU, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(LED_BIRU, LOW);
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }

  String ipString = "IP: " + WiFi.localIP().toString();
  showOLED("WiFi Connected!", ipString, "");
  delay(2000);
}


void reconnect_mqtt() {
  while (!client.connected()) {
    showOLED("Connecting MQTT", mqtt_server, "");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      for (int i = 0; i < 6; i++) {
        digitalWrite(LED_BIRU, HIGH);
        delay(100);
        digitalWrite(LED_BIRU, LOW);
        delay(100);
      }

      IPAddress mqttIP;
      String ipText = "";
      if (WiFi.hostByName(mqtt_server, mqttIP)) {
        ipText = "IP: " + mqttIP.toString();
      } else {
        ipText = "IP: Unknown";
      }
      showOLED("MQTT Connected!", mqtt_server, ipText);
      delay(2000);
    } else {
      delay(random(1500, 2500));
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_BIRU, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_BIRU, LOW);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(BUZZER, LOW);

  u8g2.begin();  
  
  delay(1000);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("LoRa Receiver");

  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    delay(100);
    while (1);
  }
}

void loop() {
  
  if (!client.connected()) {
    showOLED("MQTT Disconnected!", "Reconnecting...", "");
    reconnect_mqtt();
  }
  client.loop();

  if (client.connected()) {
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(LED_BIRU, HIGH);
  } else {
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(LED_BIRU, LOW);
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {

    lastLoRaDataTime = millis();
    String LoRaData = LoRa.readString();
    split(LoRaData,"~");
    String bmp    = myArray[0];
    String gas    = myArray[1];
    String mpu_x  = myArray[2];
    String mpu_y  = myArray[3];
    String mpu_z  = myArray[4];
    String kompas = myArray[5];
    String lat    = myArray[6];
    String lon    = myArray[7];
    String shock  = myArray[8];
    
    if(gas.toInt() > 300){
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x12_tf);
      drawCenteredStr(28, "Gas Berbahaya!");
      u8g2.sendBuffer();
      delay(1000);
    } else if(bmp.toInt() < 10){
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x12_tf);
      drawCenteredStr(28, "Denyut Tidak Ada!");
      u8g2.sendBuffer();
      delay(1000);
    } else {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x12_tf);
      drawCenteredStr(12, "BPM : "+bmp);
      drawCenteredStr(24, "Gas : "+gas);
      drawCenteredStr(36, "X:" + mpu_x + ",Y:" + mpu_y + ",Z:" + mpu_z);
      drawCenteredStr(48, "Kompas : "+kompas);
      drawCenteredStr(60, "lat:"+lat+", lon:"+lon);
      u8g2.sendBuffer();
    }

    doc["bmp"] = bmp;
    doc["gas"] = gas;
    doc["mpu_x"] = mpu_x;
    doc["mpu_y"] = mpu_y;
    doc["mpu_z"] = mpu_z;
    doc["kompas"] = kompas;
    doc["lat"] = lat;
    doc["lon"] = lon;
    doc["shock"] = shock;
    
    String jsonString;
    serializeJson(doc, jsonString);
    client.publish(mqtt_topic, jsonString.c_str());
  } 

  if (millis() - lastLoRaDataTime <= 10000) {
    unsigned long now = millis();
    if (buzzerState && now - lastBuzzerBlinkTime >= buzzerBlinkOn) {
      buzzerState = false;
      digitalWrite(BUZZER, LOW);
      digitalWrite(LED_HIJAU, LOW);
      lastBuzzerBlinkTime = now;
    } else if (!buzzerState && now - lastBuzzerBlinkTime >= buzzerBlinkOff) {
      buzzerState = true;
      digitalWrite(BUZZER, HIGH);
      digitalWrite(LED_HIJAU, HIGH);
      lastBuzzerBlinkTime = now;
    }
  } else {
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_HIJAU, LOW);
    buzzerState = false;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCenteredStr(24, "Menunggu data LoRa...");
    u8g2.sendBuffer();
  }
}

void split(String str, String dlt) {
  String data = "";
  int n = 0;str+=dlt;
  for (int i = 0; i < str.length(); ++i)
  {
    if ((String)str[i]!=dlt) data+=(String)str[i];
      else {
        myArray[n++]=data;
        data="";
      }
  }
}