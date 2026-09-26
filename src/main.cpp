#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>

// TFT pins (SPI bus 1)
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_BL 27

// Touch pins (shared with TFT SPI)
#define TOUCH_CS 33
#define TOUCH_IRQ 36

// SD Card pins (SPI bus 2)
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CLK 18
#define SD_CS 5

// RGB LED pins
#define LED_RED 4
#define LED_GREEN 17
#define LED_BLUE 16

// Calibrated touch values
#define TS_MINX 400
#define TS_MAXX 3600
#define TS_MINY 410
#define TS_MAXY 3770

TFT_eSPI tft = TFT_eSPI();
SPIClass sdSPI(VSPI);

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

void listSDCardDirectory()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("SD Card Files");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Try to mount SD card with lower frequency (400kHz for init)
  Serial.println("Attempting to mount SD card...");
  Serial.println("Using 400kHz for initialization");

  if (!SD.begin(SD_CS, sdSPI, 400000, "/sd", 5))
  {
    Serial.println("SD Card mount failed!");
    tft.setCursor(10, 40);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("SD Mount Failed!");
    tft.setCursor(10, 55);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.println("Check:");
    tft.setCursor(10, 70);
    tft.println("- Card inserted?");
    tft.setCursor(10, 85);
    tft.println("- FAT32 format?");
    tft.setCursor(10, 100);
    tft.println("- Not exFAT/NTFS?");
    tft.setCursor(10, 120);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.println("32GB cards must be");
    tft.setCursor(10, 135);
    tft.println("formatted as FAT32");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    tft.setCursor(10, 40);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("No SD card detected");
    return;
  }

  // Print card type
  Serial.print("SD Card Type: ");
  tft.setCursor(10, 40);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
    tft.print("Type: MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
    tft.print("Type: SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
    tft.print("Type: SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
    tft.print("Type: UNKNOWN");
  }

  // Print card size
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.setCursor(10, 55);
  tft.printf("Size: %lluMB", cardSize);

  // List root directory
  Serial.println("\nRoot Directory:");
  tft.setCursor(10, 75);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Root Directory:");
  tft.println("----------------");

  File root = SD.open("/");
  if (!root)
  {
    Serial.println("Failed to open directory");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error reading dir");
    return;
  }

  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  int y = 100;
  int fileCount = 0;
  File file = root.openNextFile();

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  while (file)
  {
    if (y > 310)
    {
      // Screen full
      tft.setCursor(10, 310);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.println("...(more files)");
      break;
    }

    String fileName = String(file.name());

    if (file.isDirectory())
    {
      Serial.printf("  DIR : %s\n", fileName.c_str());
      tft.setCursor(10, y);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.printf("[%s]", fileName.c_str());
    }
    else
    {
      Serial.printf("  FILE: %s (%d bytes)\n", fileName.c_str(), file.size());
      tft.setCursor(10, y);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.print(fileName);

      // Show file size
      tft.setCursor(180, y);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      if (file.size() < 1024)
      {
        tft.printf("%dB", file.size());
      }
      else if (file.size() < 1024 * 1024)
      {
        tft.printf("%dK", file.size() / 1024);
      }
      else
      {
        tft.printf("%dM", file.size() / (1024 * 1024));
      }
    }

    y += 12;
    fileCount++;
    file = root.openNextFile();
  }

  Serial.printf("\nTotal files/folders: %d\n", fileCount);

  if (fileCount == 0)
  {
    tft.setCursor(10, 100);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("(empty directory)");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP32 TFT Display + SD Card");
  Serial.println("========================================\n");

  // Initialize RGB LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  // Turn on GREEN LED to show starting
  digitalWrite(LED_GREEN, LOW);

  // Initialize backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initialize Display/Touch SPI (HSPI)
  Serial.println("Initializing Display SPI...");
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

  Serial.println("Display initialized");

  // Initialize SD Card SPI (VSPI)
  Serial.println("\nInitializing SD Card SPI...");
  Serial.printf("SD Card pins - MOSI:%d MISO:%d CLK:%d CS:%d\n",
                SD_MOSI, SD_MISO, SD_CLK, SD_CS);

  sdSPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

  // Turn GREEN off, BLUE on to show SD card reading
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, LOW);

  // Read and display SD card directory
  listSDCardDirectory();

  // Turn BLUE off, show ready (no LED)
  digitalWrite(LED_BLUE, HIGH);

  Serial.println("\n========================================");
  Serial.println("Setup complete!");
  Serial.println("Touch screen to refresh directory");
  Serial.println("\nIMPORTANT: If SD card fails to mount:");
  Serial.println("- Card MUST be formatted as FAT32");
  Serial.println("- Windows formats 32GB+ as exFAT by default");
  Serial.println("- Use Windows Disk Management to format as FAT32");
  Serial.println("- Or use SD Card Formatter tool");
  Serial.println("========================================\n");
}

void loop()
{
  static bool wasTouched = false;

  bool touched = isTouched();

  if (touched && !wasTouched)
  {
    wasTouched = true;

    // Flash RED LED
    digitalWrite(LED_RED, LOW);
    delay(100);
    digitalWrite(LED_RED, HIGH);

    Serial.println("\nRefreshing SD card directory...");

    // Flash BLUE LED while reading
    digitalWrite(LED_BLUE, LOW);
    listSDCardDirectory();
    digitalWrite(LED_BLUE, HIGH);
  }
  else if (!touched)
  {
    wasTouched = false;
  }

  delay(50);
}
