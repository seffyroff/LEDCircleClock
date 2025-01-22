#include <NeoPixelBus.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <EEPROM.h>
#include <time.h> // Replaces <TZ.h>
#include "esp_sntp.h"

#include "defines.h"

boolean timeIsSet = false;
time_t lastNtpSet = 0;
time_t currentTime = time(nullptr);
struct tm timeinfo;
time_t previousEffectTime = time(nullptr);

int previousClockSecond = -1;
int millisOffset = 0;

char ssid[60];
char wifiPassword[60];

#define RINGS 9
int ringSizes[] = {1, 8, 12, 16, 24, 32, 40, 48, 60};
int ringPowers[] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
int startLEDs[RINGS];
int totalLEDs;

const int PIXEL_COUNT = 241;
int brightness;

const long GMT_OFFSET_SEC = 0; // Replace with your timezone offset in seconds
const int DAYLIGHT_OFFSET_SEC = 3600; // Daylight saving time offset in seconds
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"

NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod> strip(PIXEL_COUNT, LED_PIN);

WebServer server(80);
HTTPUpdateServer httpUpdater;

void timeUpdated() {
  timeIsSet = true;
  lastNtpSet = time(nullptr);
  Serial.print("NTP Updated: "); 
  Serial.println(ctime(&lastNtpSet));
}

boolean isNtpOlderThanOneHour() {
  return (!timeIsSet) || (time(nullptr) - lastNtpSet) > 3620;
}

void clearStrip() {
  strip.ClearTo(RgbColor(0, 0, 0));
  strip.Show();
}

void handlingDelay(int delayMillis) {
  ArduinoOTA.handle();
  server.handleClient();
  if (delayMillis > 0) delay(delayMillis);
}

void setPixel(int pixelNumber, RgbColor color) {
  int red = min((int)color.R, MAX_BRIGHTNESS);
  int green = min((int)color.G, MAX_BRIGHTNESS);
  int blue = min((int)color.B, MAX_BRIGHTNESS);
  RgbColor limitedColor = RgbColor(red, green, blue);
  strip.SetPixelColor(pixelNumber, limitedColor);
}

void setRandomSeed() {
  uint32_t seed = analogRead(0);
  delay(1);
  for (int shifts = 3; shifts < 31; shifts += 3) {
    seed ^= analogRead(0) << shifts;
    delay(1);
  }
  randomSeed(seed);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  EEPROM.begin(512);

  strip.Begin();

  // //LED Test
  // for (int i = 0; i < PIXEL_COUNT; i++) {
  //     strip.ClearTo(RgbColor(0, 0, 0)); // Clear all LEDs
  //     strip.SetPixelColor(i, RgbColor(50, 50, 50)); // Light one LED
  //     strip.Show();
  //     delay(50);
  // }

  strip.Show();

  EEPROM.get(SSID_ADDR, ssid);
  EEPROM.get(WIFI_PASSWORD_ADDR, wifiPassword);
  Serial.print("\r\nConnecting to WIFI '");
  Serial.print(String(ssid));
  Serial.print("'...\r\n");

  strip.SetPixelColor(0, RgbColor(0, 0, 50));
  strip.Show();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(String(ssid), String(wifiPassword));
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Continuing...");
    strip.SetPixelColor(0, RgbColor(50, 0, 0));
    strip.Show();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(HOSTNAME);
  } else {
    Serial.println("Connected to WIFI...");
    strip.SetPixelColor(0, RgbColor(0, 50, 0));
    strip.Show();
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);

  setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1); // Example for UK timezone
  tzset();
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

  sntp_set_time_sync_notification_cb([](timeval *tv) {
      timeUpdated();
  });

  setupOTA();

  EEPROM.get(BRIGHTNESS_ADDR, brightness);

  httpUpdater.setup(&server);
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.begin();

  clearStrip();

  int startLED = 0;
  for (int ring = 0; ring < RINGS; ring++) {
    startLEDs[ring] = startLED;
    startLED += ringSizes[ring];
  }
  totalLEDs = startLED;

  setRandomSeed();

  Serial.println();
  Serial.println("Running...");
}

void executeEffect(int choice) {
  switch (choice) {
    case 0:
      sparkle();
      break;
    case 1:
      pacman();
      break;
    case 2:
      scan();
      break;
    case 3:
      fire();
      break;
  }
}

void loop() {
  currentTime = time(nullptr);
  localtime_r(&currentTime, &timeinfo);

  if (previousEffectTime != currentTime) {
    previousEffectTime = currentTime;
    if (random(30) == 0) {
      int effectChoice = random(4);
      executeEffect(effectChoice);
    }
  }

  ArduinoOTA.handle();
  server.handleClient();
  updateClockHands();
}
