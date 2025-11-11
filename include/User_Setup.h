#define USER_SETUP_LOADED 1

#define ST7796_DRIVER

#define TFT_WIDTH  480
#define TFT_HEIGHT 272
#define TFT_INVERSION_ON

// Pin configuration for ESP32-S3
#define TFT_MISO -1     // Not used
#define TFT_MOSI 11     // SDA
#define TFT_SCLK 12     // SCL
#define TFT_CS   10     // CS
#define TFT_DC   13     // RS/DC
#define TFT_RST  14     // RST

// Try a lower SPI frequency first to rule out timing issues
#define SPI_FREQUENCY       10000000  // Reduced from 40MHz to 10MHz
#define SPI_READ_FREQUENCY  5000000   // Reduced accordingly
#define SPI_TOUCH_FREQUENCY 2500000

// Load minimal fonts for testing
#define LOAD_GLCD
#define LOAD_FONT2

// Add explicit SPI bus configuration
#define TFT_SPI_PORT 1  // Second SPI bus on ESP32

// Force hardware SPI
#define USE_HSPI_PORT

// We're using external touch IC
#define TOUCH_CS -1