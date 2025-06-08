#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// Firebase config
#define FIREBASE_HOST "automaticfish-feeder-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "jzaLdSi2XrGgoUpI7lLPkPamNuYWf1VGWW6SXXEy"
#define WIFI_SSID "sabeink"
#define WIFI_PASSWORD "kuncengkempus123"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200); // GMT+7

Servo servo;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String Str[] = {"00:00", "00:00", "00:00"};
String stimer;
int i;
int feednow = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected with IP: ");
  Serial.println(WiFi.localIP());

  // Mulai sinkronisasi waktu
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // Konfigurasi Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  config.cert.data = NULL;  // Matikan SSL certificate (wajib agar stabil)
  config.time_zone = 7;     // WIB

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  servo.attach(13); // GPIO13 untuk sinyal servo
}

void loop() {
  // Feed Now
  if (Firebase.RTDB.getInt(&fbdo, "/feednow")) {
    feednow = fbdo.intData();
    Serial.println("feednow: " + String(feednow));
    if (feednow == 1) {
      servo.write(90);  // putar penuh
      delay(700);
      servo.write(0);    // kembali ke awal
      Firebase.RTDB.setInt(&fbdo, "/feednow", 0);
      Serial.println("Fed (manual)");
    }
  }

  // Jadwal Timer
  for (i = 0; i < 3; i++) {
    String path = "/timers/timer" + String(i) + "/time";
    if (Firebase.RTDB.getString(&fbdo, path)) {
      stimer = fbdo.stringData();
      Str[i] = stimer.substring(0, 5); // "08:00"
    } else {
      Str[i] = "00:00";
    }
  }

  timeClient.update();
  String h = String(timeClient.getHours());
  String m = String(timeClient.getMinutes());
  if (h.length() == 1) h = "0" + h;
  if (m.length() == 1) m = "0" + m;
  String currentTime = h + ":" + m;
  Serial.println("Now: " + currentTime);

  if (Str[0] == currentTime || Str[1] == currentTime || Str[2] == currentTime) {
    servo.write(90);
    delay(700);
    servo.write(0);
    Serial.println("Fed (scheduled)");
    delay(60000); // tunggu 1 menit
  }

  delay(1000);
}