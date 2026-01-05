#include <Arduino.h>
#include <Arduino_Modulino.h>
#include <Arduino_LED_Matrix.h>
#include <ArduinoGraphics.h>

ModulinoThermo thermo;
ModulinoBuzzer buzzer;
ArduinoLEDMatrix matrix;

//buzzer variables
int frequency = 840; // Frequency of the tone in Hz
int duration = 1000; // Duration of the tone in milliseconds

//cap. sensor constants
const int dry = 500; //value for dry sensor (0%)
const int wet = 230; //constant for wet sensor (100%)

void setup()
{
  Serial.begin(9600);

  // led matrix
  matrix.begin();

  // modulino
  Modulino.begin();
  thermo.begin();
  buzzer.begin();
}

void loop()
{
  // air humidity
  int airHumidity = thermo.getHumidity();
  //cap. sensors
  int sensorVal1 = analogRead(A0);
  int sensorVal2 = analogRead(A1);
  int sensorVal3 = analogRead(A2);
  int soilHumidity1 = map(sensorVal1, wet, dry, 100, 0);
  int soilHumidity2 = map(sensorVal2, wet, dry, 100, 0);
  int soilHumidity3 = map(sensorVal3, wet, dry, 100, 0);

  // led matrix
  uint32_t white = 0xFFFFFFFF;
  matrix.beginDraw();
  matrix.stroke(white);
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, white);
  matrix.print(airHumidity);
  matrix.print("%");
  matrix.endText();
  matrix.endDraw();

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

  // air humidity alarm over 60%
  if (airHumidity > 60)
  {
    for (int i = 0; i < 3; ++i)
    {
      buzzer.tone(frequency, duration);
      delay(3000);
    }
    buzzer.tone(0, 0);
    delay(60000);
  }
}