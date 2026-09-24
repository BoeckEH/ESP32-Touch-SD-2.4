# ESP32 2.4" TFT Touch Display Project

## Hardware Configuration

### Board
- **Type:** ESP32-WROOM-32 (ESP32 original, dual-core Xtensa)
- **NOT:** ESP32-C6 (which is RISC-V based)
- **PlatformIO Board:** `esp32dev`

### Display - ST7789
- **Driver:** ST7789 (240x320)
- **Size:** 2.4 inches
- **Resolution:** 240 x 320 pixels
- **Color Inversion:** OFF (false)

#### Display Pin Configuration
```cpp
#define TFT_MISO   12
#define TFT_MOSI   13
#define TFT_SCLK   14
#define TFT_CS     15
#define TFT_DC     2     // Critical: GPIO 2 (not 5)
#define TFT_RST    -1    // Not connected
#define TFT_BL     27    // Backlight (discovered via testing)
```

### Touch Controller - XPT2046
- **Type:** XPT2046 Resistive Touch
- **Connection:** Shares SPI bus with display
- **Communication:** Uses same CLK/MISO/MOSI as display, separate CS

#### Touch Pin Configuration
```cpp
#define TOUCH_CS   33    // Touch chip select
#define TOUCH_IRQ  36    // Touch interrupt (active LOW when touched)
// Shared with display:
// TOUCH_CLK  = 14 (same as TFT_SCLK)
// TOUCH_DIN  = 13 (same as TFT_MOSI)
// TOUCH_DOUT = 12 (same as TFT_MISO)
```

#### Touch Calibration Values
Determined by testing all four corners:

```cpp
#define TS_MINX 400
#define TS_MAXX 3600
#define TS_MINY 410
#define TS_MAXY 3770
```

#### Touch Coordinate Mapping
```cpp
// X is reversed, Y is normal
int x = map(rawX, TS_MAXX, TS_MINX, 0, 240);  // High raw X = left
int y = map(rawY, TS_MINY, TS_MAXY, 0, 320);  // Low raw Y = top
```

### RGB LED
- **Type:** Common Anode (Active LOW)
- **Control:** Set pin LOW to turn LED ON, HIGH to turn OFF

#### LED Pin Configuration
```cpp
#define LED_RED    4     // GPIO 4
#define LED_GREEN  17    // GPIO 17
#define LED_BLUE   16    // GPIO 16
```

## PlatformIO Configuration

### platformio.ini
```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

; Libraries
lib_deps =
    bodmer/TFT_eSPI@^2.5.43
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git

; TFT_eSPI configuration
build_flags =
    -D USER_SETUP_LOADED=1
    -D ST7789_DRIVER=1
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=320
    -D TFT_MISO=12
    -D TFT_MOSI=13
    -D TFT_SCLK=14
    -D TFT_CS=15
    -D TFT_DC=2
    -D TFT_RST=-1
    -D TFT_BL=27
    -D LOAD_GLCD=1
    -D LOAD_FONT2=1
    -D LOAD_FONT4=1
    -D LOAD_FONT6=1
    -D LOAD_FONT7=1
    -D LOAD_FONT8=1
    -D LOAD_GFXFF=1
    -D SMOOTH_FONT=1
    -D SPI_FREQUENCY=27000000
    -D SPI_READ_FREQUENCY=20000000
```

## Key Discoveries

### 1. Backlight Pin Discovery
Initially specified as GPIO 21, but discovered to be **GPIO 27** through systematic GPIO scanning.

### 2. Display Driver
Package labeled "7789" - confirmed to be **ST7789** (not ILI9341).
- No color inversion needed
- Works at 27 MHz SPI frequency

### 3. Touch Controller
Resistive touch (XPT2046), **NOT** capacitive (CST820).
- Shares SPI bus with display
- Requires SPI frequency switching (2 MHz for touch, 27 MHz for display)
- IRQ pin goes LOW when screen is touched

### 4. Touch Reading Method
Due to library limitations, implemented **raw SPI reading**:
```cpp
uint16_t readTouchRaw(uint8_t command)
{
  digitalWrite(TOUCH_CS, LOW);
  delayMicroseconds(10);
  SPI.transfer(command);
  delayMicroseconds(10);
  data = SPI.transfer(0x00) << 8;
  data |= SPI.transfer(0x00);
  data >>= 3; // 12-bit value
  digitalWrite(TOUCH_CS, HIGH);
  return data;
}

// Commands:
// 0x90 = Read X position
// 0xD0 = Read Y position
// 0xB0 = Read Z (pressure)
```

### 5. RGB LED Control
LEDs are **active-low** (common anode):
```cpp
digitalWrite(LED_RED, LOW);   // Turn RED on
digitalWrite(LED_RED, HIGH);  // Turn RED off
```

## Calibration Process

### Touch Calibration Steps
1. Drew colored circles at four corners
2. Touched each corner clockwise from top-left
3. Recorded raw X/Y values:
   - Top Left (10,10): Raw X=3502, Y=421
   - Top Right (230,10): Raw X=424, Y=408
   - Bottom Right (230,310): Raw X=396, Y=3760
   - Bottom Left (10,310): Raw X=3697, Y=3775

4. Calculated min/max ranges:
   - X: 400 to 3600 (reversed mapping)
   - Y: 410 to 3770 (normal mapping)

## Working Example Code

See `src/main.cpp` for complete working implementation including:
- Display initialization with ST7789 driver
- Touch reading with XPT2046
- Coordinate mapping and calibration
- Drawing interface with touch-to-draw functionality
- Clear button functionality
- RGB LED cycling with color identification

## Troubleshooting Notes

### Issues Encountered and Solutions

1. **Display blank despite program running:**
   - **Solution:** Wrong backlight pin (was 21, actually 27)
   - Found by cycling through all GPIO pins

2. **Touch not responding:**
   - **Solution:** Touch shares SPI bus with display, not separate bus
   - Required SPI frequency switching between display and touch operations

3. **Touch coordinates reversed/inverted:**
   - **Solution:** X coordinate needed reverse mapping (MAXX to MINX)
   - Calibrated using corner touch test

4. **Color inversion wrong:**
   - **Initial:** Tried with inversion ON
   - **Solution:** ST7789 works with inversion OFF

5. **RGB LED not turning on:**
   - **Solution:** LEDs are active-LOW, not active-HIGH

## Pin Summary Table

| Function | GPIO | Notes |
|----------|------|-------|
| **Display** ||||
| TFT_MISO | 12 | Shared with touch |
| TFT_MOSI | 13 | Shared with touch |
| TFT_SCLK | 14 | Shared with touch |
| TFT_CS | 15 | Display chip select |
| TFT_DC | 2 | Data/Command |
| TFT_RST | -1 | Not connected |
| TFT_BL | 27 | Backlight control |
| **Touch** ||||
| TOUCH_CS | 33 | Touch chip select |
| TOUCH_IRQ | 36 | Touch interrupt |
| **RGB LED** ||||
| LED_RED | 4 | Active LOW |
| LED_GREEN | 17 | Active LOW |
| LED_BLUE | 16 | Active LOW |

## Libraries Used

- **TFT_eSPI** v2.5.43 by Bodmer
- **XPT2046_Touchscreen** by Paul Stoffregen (via GitHub)
- **SPI** (Arduino built-in)
- **Wire** (Arduino built-in, for I2C scanning during diagnostics)

## Build Instructions

1. Install PlatformIO
2. Clone/create project with above configuration
3. Build: `pio run`
4. Upload: `pio run -t upload`
5. Monitor: `pio device monitor`

## Testing Checklist

- [x] Display shows colors correctly
- [x] Backlight controls brightness
- [x] Touch responds to finger press
- [x] Touch coordinates accurately mapped
- [x] Drawing works smoothly
- [x] Clear button functions
- [x] RGB LED cycles through colors
- [x] All three LED colors identified

## Date
Session completed: 2026-09-24

---
*Documentation generated during Claude Code debugging session*
