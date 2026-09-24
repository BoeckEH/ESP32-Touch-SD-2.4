#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// TFT pins
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_BL 27

// Touch pins (shared SPI)
#define TOUCH_CS 33
#define TOUCH_IRQ 36

// RGB LED pins
#define LED_PIN1 17
#define LED_PIN2 4
#define LED_PIN3 16

// Calibrated touch values
#define TS_MINX 400
#define TS_MAXX 3600
#define TS_MINY 410
#define TS_MAXY 3770

TFT_eSPI tft = TFT_eSPI();

// Manual XPT2046 reading
uint16_t readTouchRaw(uint8_t command)
{
  uint16_t data = 0;

  digitalWrite(TOUCH_CS, LOW);
  delayMicroseconds(10);

  SPI.transfer(command);
  delayMicroseconds(10);

  data = SPI.transfer(0x00) << 8;
  data |= SPI.transfer(0x00);
  data >>= 3;

  digitalWrite(TOUCH_CS, HIGH);
  delayMicroseconds(10);

  return data;
}

bool isTouched()
{
  return digitalRead(TOUCH_IRQ) == LOW;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP32 TFT Display & Touch + RGB LED");
  Serial.println("========================================\n");

  // Initialize RGB LED pins
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LED_PIN3, OUTPUT);
  digitalWrite(LED_PIN1, LOW);
  digitalWrite(LED_PIN2, LOW);
  digitalWrite(LED_PIN3, LOW);

  Serial.println("RGB LED pins: 17, 4, 16");
  Serial.println("Cycling every 1 second...\n");

  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initialize SPI
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  SPI.setFrequency(2000000);

  // Setup touch pins
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_IRQ, INPUT);

  // Initialize display
  SPI.setFrequency(27000000);
  tft.init();
  tft.invertDisplay(false);
  tft.setRotation(0);

  // Draw interface
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("Touch Test");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);
  tft.println("ST7789 - 240x320");
  tft.setCursor(10, 55);
  tft.println("Touch calibrated!");

  // Draw touch area
  tft.fillRect(10, 80, 220, 100, TFT_BLUE);
  tft.setTextColor(TFT_YELLOW, TFT_BLUE);
  tft.setTextSize(3);
  tft.setCursor(30, 115);
  tft.println("TOUCH");
  tft.setCursor(40, 145);
  tft.println("HERE");

  // Clear button
  tft.fillRect(10, 280, 220, 30, TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(2);
  tft.setCursor(70, 287);
  tft.println("CLEAR");

  // Coordinate display area
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 195);
  tft.println("Touch coordinates:");

  // LED status area
  tft.setCursor(10, 245);
  tft.println("RGB LED:");
}

void loop()
{
  static bool wasTouched = false;
  static int lastX = -1;
  static int lastY = -1;
  static unsigned long lastLEDChange = 0;
  static int ledState = 0;

  // RGB LED cycling (1 second intervals)
  if (millis() - lastLEDChange > 1000)
  {
    // Turn off all LEDs
    digitalWrite(LED_PIN1, HIGH); // 17 is Green
    digitalWrite(LED_PIN2, HIGH); // 4 is Red
    digitalWrite(LED_PIN3, HIGH); // 16 is Blue

    // Cycle through LEDs
    switch (ledState)
    {
    case 0:
      digitalWrite(LED_PIN1, HIGH);
      digitalWrite(LED_PIN2, HIGH);
      digitalWrite(LED_PIN3, HIGH);
      digitalWrite(LED_PIN1, LOW);
      Serial.println("LED: GPIO 17 ON");
      tft.fillRect(10, 260, 220, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(10, 260);
      tft.println("GPIO 17 ON");
      break;
    case 1:
      digitalWrite(LED_PIN1, HIGH);
      digitalWrite(LED_PIN2, HIGH);
      digitalWrite(LED_PIN3, HIGH);
      digitalWrite(LED_PIN2, LOW);
      Serial.println("LED: GPIO 4 ON");
      tft.fillRect(10, 260, 220, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(10, 260);
      tft.println("GPIO 4 ON");
      break;
    case 2:
      digitalWrite(LED_PIN1, HIGH);
      digitalWrite(LED_PIN2, HIGH);
      digitalWrite(LED_PIN3, HIGH);
      digitalWrite(LED_PIN3, LOW);
      Serial.println("LED: GPIO 16 ON");
      tft.fillRect(10, 260, 220, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(10, 260);
      tft.println("GPIO 16 ON");
      break;
    }

    ledState = (ledState + 1) % 3;
    lastLEDChange = millis();
  }

  // Touch handling
  bool touched = isTouched();

  if (touched)
  {
    if (!wasTouched)
    {
      wasTouched = true;
    }

    // Read touch coordinates
    SPI.setFrequency(2000000);

    uint16_t rawX = readTouchRaw(0x90);
    delay(1);
    uint16_t rawY = readTouchRaw(0xD0);
    delay(1);
    uint16_t rawZ = readTouchRaw(0xB0);

    SPI.setFrequency(27000000);

    // Map to screen with calibrated values
    int x = map(rawX, TS_MAXX, TS_MINX, 0, 240); // X reversed
    int y = map(rawY, TS_MINY, TS_MAXY, 0, 320); // Y normal
    x = constrain(x, 0, 239);
    y = constrain(y, 0, 319);

    // Display coordinates
    tft.fillRect(10, 210, 220, 30, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 210);
    tft.printf("Raw: X=%4d Y=%4d", rawX, rawY);
    tft.setCursor(10, 225);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("Screen: X=%3d Y=%3d", x, y);

    // Check for clear button press
    if (y >= 280 && y <= 310 && x >= 10 && x <= 230)
    {
      // Clear drawing area
      tft.fillRect(0, 0, 240, 275, TFT_BLACK);

      // Redraw interface
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(20, 10);
      tft.println("Touch Test");

      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(10, 40);
      tft.println("ST7789 - 240x320");
      tft.setCursor(10, 55);
      tft.println("Touch calibrated!");

      tft.fillRect(10, 80, 220, 100, TFT_BLUE);
      tft.setTextColor(TFT_YELLOW, TFT_BLUE);
      tft.setTextSize(3);
      tft.setCursor(30, 115);
      tft.println("TOUCH");
      tft.setCursor(40, 145);
      tft.println("HERE");

      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(10, 195);
      tft.println("Touch coordinates:");

      tft.setCursor(10, 245);
      tft.println("RGB LED:");

      lastX = -1;
      lastY = -1;
    }
    else
    {
      // Draw on screen
      tft.fillCircle(x, y, 2, TFT_WHITE);

      // Draw line from last point
      if (lastX != -1 && lastY != -1)
      {
        tft.drawLine(lastX, lastY, x, y, TFT_WHITE);
      }

      lastX = x;
      lastY = y;
    }

    delay(10);
  }
  else
  {
    wasTouched = false;
    lastX = -1;
    lastY = -1;
  }

  delay(10);
}
