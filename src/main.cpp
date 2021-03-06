#include <Arduino.h>
#include <SPI.h> //uključujemo SPI biblioteku koja nam omogućuje SPI komunikaciju
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_SSD1306.h> //uključujemo biblioteku za rad s OLED displejom
#include <Adafruit_I2CDevice.h>
#include "printf.h"

#define BT1 2
#define BT2 3
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_SDA 5       // SDA - MOSI
#define OLED_SCL 4       // SCL - CLK
#define OLED_DC 9        // DC or AO
#define OLED_CS 10
#define OLED_RESET 6 //RST

int channel = 0;
bool jamming = false;
unsigned long previousMillis = 0;
const long interval = 1000;

void pressBt01();
void pressBt02();
void jammer();
void renderGraph();
void smartJammer();

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_SDA, OLED_SCL, OLED_DC, OLED_RESET, OLED_CS);

RF24 radio(7, 8); // CE, CSN
const uint8_t num_channels = 64;
int values[num_channels];
int valuesDisplay[32];
const byte address[6] = "00001";
const int num_reps = 50;

void setup()
{
  Serial.begin(9600);
  pinMode(BT1, INPUT_PULLUP);
  pinMode(BT2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BT1), pressBt01, FALLING);
  attachInterrupt(digitalPinToInterrupt(BT2), pressBt02, FALLING);

  radio.begin();
  radio.startListening();
  radio.setContWave();
  radio.setAutoAck(false);
  radio.stopListening();
  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
}
void loop()
{
  renderGraph();

  if (jamming)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.println(("JAMMING CHANNEL: " + String(channel + 1)));
    radio.setPALevel(RF24_PA_HIGH);
    radio.setDataRate(RF24_2MBPS);
    display.display();
  }
  while (jamming)
  {
    jammer();
    //smartJammer();
  }
}

void jammer()
{
  const char text[] = "xhgfmjtzrxudrhe43tz"; // send the noise

  for (int i = ((channel * 5) + 1); i < ((channel * 5) + 23); i++)
  {
    radio.setChannel(i);
    radio.write(&text, sizeof(text));
  }
}

void renderGraph()
{
  memset(values, 0, sizeof(values));

  // Scan all channels num_reps times
  int rep_counter = num_reps;
  while (rep_counter--)
  {
    int i = num_channels;
    while (i--)
    {
      // Select this channel
      radio.setChannel(i * 2);
      // Listen for a little
      radio.startListening();
      delayMicroseconds(100);
      radio.stopListening();

      // Did we get a carrier?
      if (radio.testCarrier())
        ++values[i];
    }
  }
  // Clear the buffer
  display.clearDisplay();
  display.drawLine(0, 50, 127, 50, WHITE);

  // Adjusting the bars to correct resolution of the display
  for (int i = 0; i < 64; i++)
  {
    display.fillRect((1 + (i * 2)), (50 - values[i]), 2, values[i], WHITE);
  }

  // Drawing the bottom lines
  display.drawLine(0, 55, 0, 60, WHITE);
  display.drawLine(15, 55, 15, 60, WHITE);
  display.drawLine(31, 55, 31, 60, WHITE);
  display.drawLine(47, 55, 47, 60, WHITE);
  display.drawLine(63, 55, 63, 60, WHITE);
  display.drawLine(80, 55, 80, 60, WHITE);
  display.drawLine(95, 55, 95, 60, WHITE);
  display.drawLine(111, 55, 111, 60, WHITE);
  display.drawLine(127, 55, 127, 60, WHITE);

  // Show selected channel to start jam
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(10, 10);   // Start at top-left corner
  display.print("channel: " + String(channel + 1));
  display.display();
}

void pressBt01()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200)
  {
    if (channel < 13)
    {
      channel++;
    }
    else
    {
      channel = 0;
    }
  }
  last_interrupt_time = interrupt_time;
}

void pressBt02()
{
  delay(200);
  jamming = !jamming;
  delay(200);
  //radio.stopListening();
}

void smartJammer()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    if (channel == 0)
    {
      channel = 5;
    }
    else if (channel == 5)
    {
      channel = 10;
    }
    else
    {
      channel = 0;
    }
  }

  const char text[] = "xhgfmjtzrxudrhe43tz"; // send the noise
  for (int i = ((channel * 5) + 1); i < ((channel * 5) + 23); i++)
  {
    radio.setChannel(i);
    radio.write(&text, sizeof(text));
  }
}