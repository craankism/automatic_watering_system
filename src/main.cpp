#include <Arduino.h>
#include <Arduino_Modulino.h>
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <RTC.h>
#include <Arduino.h>
#include <Arduino_Modulino.h>
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <RTC.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <credentials.h>
#include <iostream>
#include <string>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

ModulinoThermo thermo;
ModulinoBuzzer buzzer;
ArduinoLEDMatrix matrix;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// buzzer variables
int frequency = 840; // Frequency of the tone in Hz
int duration = 1000; // Duration of the tone in milliseconds

// cap. sensor constants
const int dry = 500; // value for dry sensor (0%)
const int wet = 230; // constant for wet sensor (100%)

// air humidity
int airHumidity;

// cap. sensors
int sensorVal1;
int sensorVal2;
int sensorVal3;
int soilHumidity1;
int soilHumidity2;
int soilHumidity3;

const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASS;

int wifiStatus = WL_IDLE_STATUS;

// display toggle (non-blocking)
int displayPage = 0;
unsigned long lastDisplaySwitch = 0;
unsigned long lastDisplayRefresh = 0;
const unsigned long displayRefreshInterval = 500UL; // ms (refresh while page shown)
const unsigned long displayInterval = 5000UL;       // ms

void alarm(int airHumidity);
void screen(const String &printDisplay1, const String &printDisplay2, const String &printDisplay3);
void updateReadings();

void setup()
{
  Serial.begin(9600);

  // led matrix
  matrix.begin();

  // modulino
  Modulino.begin();
  thermo.begin();
  buzzer.begin();

  // display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // WiFi
  screen("Connecting to WiFi:", ssid, "");
  WiFi.begin(ssid, password);
  screen("WiFi connected", "", "");
  delay(2000);

  // Time from WIFI
  timeClient.begin();
}

void loop()
{
  // update sensors and time once per loop (will also refresh again before drawing)
  updateReadings();

  // alarm RH > 60%
  alarm(airHumidity);

  // non-blocking display: switch pages every `displayInterval` ms,
  // but refresh the currently visible content every `displayRefreshInterval` ms
  unsigned long now = millis();

  // first-time init
  if (lastDisplaySwitch == 0)
  {
    lastDisplaySwitch = lastDisplayRefresh = now;
    displayPage = 0;
  }

  bool switchPage = (now - lastDisplaySwitch) >= displayInterval;
  bool refreshPage = (now - lastDisplayRefresh) >= displayRefreshInterval;

  if (switchPage)
  {
    lastDisplaySwitch = now;
    lastDisplayRefresh = now;
    displayPage = !displayPage; // toggle 0 <-> 1
  }

  if (switchPage || refreshPage)
  {
    lastDisplayRefresh = now;
    updateReadings();

    if (displayPage == 0)
    {
      screen("", timeClient.getFormattedTime(), "");
    }
    else
    {
      screen("Luftfeuchte: ", String(airHumidity), "%");
    }
  }
}

void alarm(int airHumidity)
{
  // Non-blocking alarm using millis() to avoid delaying the main loop.
  // When air humidity > 60% starts a sequence of 3 pulses spaced by pulseInterval.
  static bool active = false;
  static unsigned long lastMillis = 0;
  static int pulseCount = 0;
  static unsigned long cooldownUntil = 0;         // timestamp until which re-trigger is blocked
  const unsigned long pulseInterval = 3000UL;     // time between pulse starts (ms)
  const unsigned long cooldownDuration = 60000UL; // 1 minute cooldown after sequence (ms)

  unsigned long now = millis();

  // If we're in cooldown after a full sequence, don't start a new one
  if (now < cooldownUntil)
  {
    if (active)
    {
      active = false;
      buzzer.tone(0, 0);
    }
    return;
  }

  // Start alarm sequence when threshold exceeded
  if (airHumidity > 60)
  {
    if (!active)
    {
      active = true;
      pulseCount = 0;
      lastMillis = now - pulseInterval; // allow immediate first pulse
    }
  }
  else
  {
    // Stop alarm immediately when humidity drops
    if (active)
    {
      active = false;
      buzzer.tone(0, 0);
    }
    return;
  }

  if (!active)
    return;

  if (pulseCount < 3 && (now - lastMillis) >= pulseInterval)
  {
    buzzer.tone(frequency, duration);
    lastMillis = now;
    ++pulseCount;
  }

  if (pulseCount > 3)
  {
    // finished sequence: stop buzzer and set cooldown so alarm won't restart for 1 minute
    active = false;
    buzzer.tone(0, 0);
    cooldownUntil = now + cooldownDuration;
  }
}

void screen(const String &printDisplay1, const String &printDisplay2, const String &printDisplay3)
{
  String text = printDisplay1 + printDisplay2 + printDisplay3;

  int16_t x1, y1;
  uint16_t w, h;

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(64 - w / 2, 16 - h / 2);
  display.print(text);
  display.display();
}

void updateReadings()
{
  timeClient.update();
  airHumidity = thermo.getHumidity();
  sensorVal1 = analogRead(A0);
  sensorVal2 = analogRead(A1);
  sensorVal3 = analogRead(A2);
  soilHumidity1 = map(sensorVal1, wet, dry, 100, 0);
  soilHumidity2 = map(sensorVal2, wet, dry, 100, 0);
  soilHumidity3 = map(sensorVal3, wet, dry, 100, 0);
}