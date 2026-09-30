#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SdFat.h>

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

// SdFat for exFAT support
SdFs sd;
FsFile root;
FsFile file;

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

  // Configure SdFat for custom SPI
  SdSpiConfig spiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(4), &sdSPI);

  Serial.println("Attempting to mount SD card...");
  Serial.println("SdFat library - supports FAT16/FAT32/exFAT");

  if (!sd.begin(spiConfig))
  {
    Serial.println("SD Card mount failed!");
    Serial.println("Error details:");
    sd.initErrorPrint(&Serial);

    tft.setCursor(10, 40);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("SD Mount Failed!");
    tft.setCursor(10, 55);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.println("Check:");
    tft.setCursor(10, 70);
    tft.println("- Card inserted?");
    tft.setCursor(10, 85);
    tft.println("- Card formatted?");
    tft.setCursor(10, 100);
    tft.println("Supports:");
    tft.setCursor(10, 115);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.println("FAT16/FAT32/exFAT");
    return;
  }

  // Get card type
  uint8_t cardType = sd.card()->type();
  Serial.print("SD Card Type: ");
  tft.setCursor(10, 40);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  switch (cardType)
  {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      tft.print("Type: SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      tft.print("Type: SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC/SDXC");
      tft.print("Type: SDHC/SDXC");
      break;
    default:
      Serial.println("Unknown");
      tft.print("Type: Unknown");
  }

  // Get filesystem type
  tft.setCursor(10, 55);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  if (sd.fatType() == FAT_TYPE_EXFAT)
  {
    Serial.println("Filesystem: exFAT");
    tft.print("FS: exFAT");
  }
  else if (sd.fatType() == FAT_TYPE_FAT32)
  {
    Serial.println("Filesystem: FAT32");
    tft.print("FS: FAT32");
  }
  else if (sd.fatType() == FAT_TYPE_FAT16)
  {
    Serial.println("Filesystem: FAT16");
    tft.print("FS: FAT16");
  }

  // Print card size
  uint64_t cardSize = sd.card()->sectorCount() * 512ULL / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.setCursor(10, 70);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.printf("Size: %lluMB", cardSize);

  // List root directory
  Serial.println("\nRoot Directory:");
  tft.setCursor(10, 90);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Root Directory:");
  tft.println("----------------");

  if (!root.open("/"))
  {
    Serial.println("Failed to open root directory");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error reading dir");
    return;
  }

  int y = 115;
  int fileCount = 0;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  while (file.openNext(&root, O_RDONLY))
  {
    if (y > 310)
    {
      // Screen full
      tft.setCursor(10, 310);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.println("...(more files)");
      file.close();
      break;
    }

    char fileName[64];
    file.getName(fileName, sizeof(fileName));

    if (file.isDirectory())
    {
      Serial.printf("  DIR : %s\n", fileName);
      tft.setCursor(10, y);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.printf("[%s]", fileName);
    }
    else
    {
      uint32_t fileSize = file.fileSize();
      Serial.printf("  FILE: %s (%lu bytes)\n", fileName, fileSize);
      tft.setCursor(10, y);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.print(fileName);

      // Show file size
      tft.setCursor(180, y);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      if (fileSize < 1024)
      {
        tft.printf("%luB", fileSize);
      }
      else if (fileSize < 1024 * 1024)
      {
        tft.printf("%luK", fileSize / 1024);
      }
      else
      {
        tft.printf("%luM", fileSize / (1024 * 1024));
      }
    }

    y += 12;
    fileCount++;
    file.close();
  }

  root.close();

  Serial.printf("\nTotal files/folders: %d\n", fileCount);

  if (fileCount == 0)
  {
    tft.setCursor(10, 115);
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
  Serial.println("\nSD Card Support:");
  Serial.println("- FAT16/FAT32/exFAT supported!");
  Serial.println("- Using SdFat library for exFAT");
  Serial.println("- Works with cards formatted by Windows");
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
