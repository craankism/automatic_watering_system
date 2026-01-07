#include <Arduino.h>
#include <Arduino_Modulino.h>
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

ModulinoThermo thermo;
ModulinoBuzzer buzzer;
ArduinoLEDMatrix matrix;

// buzzer variables
int frequency = 840; // Frequency of the tone in Hz
int duration = 1000; // Duration of the tone in milliseconds

// cap. sensor constants
const int dry = 500; // value for dry sensor (0%)
const int wet = 230; // constant for wet sensor (100%)

void alarm(int airHumidity);

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
}

void loop()
{
  // air humidity
  int airHumidity = thermo.getHumidity();

  // cap. sensors
  int sensorVal1 = analogRead(A0);
  int sensorVal2 = analogRead(A1);
  int sensorVal3 = analogRead(A2);
  int soilHumidity1 = map(sensorVal1, wet, dry, 100, 0);
  int soilHumidity2 = map(sensorVal2, wet, dry, 100, 0);
  int soilHumidity3 = map(sensorVal3, wet, dry, 100, 0);

  // alarm RH > 60%
  alarm(airHumidity);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.print("Air Humidity: ");
  display.print(airHumidity);
  display.print("%");
  display.display();

  /* led matrix
  uint32_t white = 0xFFFFFFFF;
  matrix.beginDraw();
  matrix.stroke(white);
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, white);
  matrix.print(airHumidity);
  matrix.print("%");
  matrix.endText();
  matrix.endDraw();
  */

  // serial monitor
  Serial.print("Air Humidity is: ");
  Serial.print(airHumidity);
  Serial.println("%");
  Serial.print("Soil Humidity is: ");
  Serial.print(soilHumidity1);
  Serial.print("% on Ginseng Bonsai | ");
  Serial.print(soilHumidity2);
  Serial.print("% on Drachenbaum | ");
  Serial.print(soilHumidity3);
  Serial.println("% on Ficus Bonsai");
  delay(3000);
}

void alarm(int airHumidity)
{
  // Non-blocking alarm using millis() to avoid delaying the main loop.
  // When air humidity > 60% starts a sequence of 3 pulses spaced by pulseInterval.
  static bool active = false;
  static unsigned long lastMillis = 0;
  static int pulseCount = 0;
  const unsigned long pulseInterval = 3000UL; // time between pulse starts (ms)

  // Start alarm sequence when threshold exceeded
  if (airHumidity > 60)
  {
    if (!active)
    {
      active = true;
      pulseCount = 0;
      lastMillis = millis() - pulseInterval; // allow immediate first pulse
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

  unsigned long now = millis();
  if (pulseCount < 3 && (now - lastMillis) >= pulseInterval)
  {
    buzzer.tone(frequency, duration);
    lastMillis = now;
    ++pulseCount;
  }

  if (pulseCount >= 3)
  {
    active = false;
    buzzer.tone(0, 0);
  }
}
