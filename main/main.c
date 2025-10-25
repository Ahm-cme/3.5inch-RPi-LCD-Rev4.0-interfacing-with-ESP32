#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Pin definitions
#define TFT_MOSI    23
#define TFT_SCLK    18
#define TFT_CS      5
#define TFT_DC      21
#define TFT_RST     4

// Display dimensions (landscape)
#define TFT_WIDTH   480
#define TFT_HEIGHT  320

// ILI9486 Commands
#define ILI9486_NOP         0x00
#define ILI9486_SWRESET     0x01
#define ILI9486_RDDID       0x04
#define ILI9486_RDDST       0x09
#define ILI9486_SLPIN       0x10
#define ILI9486_SLPOUT      0x11
#define ILI9486_PTLON       0x12
#define ILI9486_NORON       0x13
#define ILI9486_INVOFF      0x20
#define ILI9486_INVON       0x21
#define ILI9486_GAMMASET    0x26
#define ILI9486_DISPOFF     0x28
#define ILI9486_DISPON      0x29
#define ILI9486_CASET       0x2A
#define ILI9486_PASET       0x2B
#define ILI9486_RAMWR       0x2C
#define ILI9486_RAMRD       0x2E
#define ILI9486_MADCTL      0x36
#define ILI9486_PIXFMT      0x3A
#define ILI9486_FRMCTR1     0xB1
#define ILI9486_FRMCTR2     0xB2
#define ILI9486_FRMCTR3     0xB3
#define ILI9486_INVCTR      0xB4
#define ILI9486_DFUNCTR     0xB6
#define ILI9486_PWCTR1      0xC0
#define ILI9486_PWCTR2      0xC1
#define ILI9486_PWCTR3      0xC2
#define ILI9486_PWCTR4      0xC3
#define ILI9486_PWCTR5      0xC4
#define ILI9486_VMCTR1      0xC5
#define ILI9486_VMCTR2      0xC7
#define ILI9486_GMCTRP1     0xE0
#define ILI9486_GMCTRN1     0xE1

// RGB565 Colors
#define TFT_BLACK       0x0000
#define TFT_BLUE        0x001F
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF

static const char *TAG = "TFT";
static spi_device_handle_t spi;

// ===== Low-level SPI functions =====

static void tft_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(TFT_DC, dc);
}

void tft_cmd(uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;  // DC low for command
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void tft_data(uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    t.user = (void*)1;  // DC high for data
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void tft_data_buf(uint8_t *data, int len)
{
    if (len == 0) return;
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    t.user = (void*)1;  // DC high for data
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

// ===== Hardware initialization =====

void tft_init_pins(void)
{
    // Configure DC and RST pins
    gpio_set_direction(TFT_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(TFT_RST, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "GPIO pins configured");
}

void tft_reset(void)
{
    ESP_LOGI(TAG, "Hardware reset...");
    gpio_set_level(TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
}

void tft_init_spi(void)
{
    esp_err_t ret;

    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = TFT_MOSI,
        .sclk_io_num = TFT_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TFT_WIDTH * 2 * 20,  // 20 lines buffer
        .flags = 0
    };

    // Initialize SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,  // Start slow: 10 MHz
        .mode = 0,                           // SPI mode 0
        .spics_io_num = TFT_CS,
        .queue_size = 7,
        .pre_cb = tft_spi_pre_transfer_callback,
        .flags = 0
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "SPI initialized at 10 MHz");
}

// ===== Display initialization (COMPLETE SEQUENCE) =====

void tft_init_display(void)
{
    ESP_LOGI(TAG, "Starting ILI9486 initialization...");

    // Software Reset
    tft_cmd(ILI9486_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    // Interface Mode Control (specific to ILI9486)
    tft_cmd(0xB0);
    tft_data(0x00);  // SDO NOT USE
    
    // Sleep Out
    tft_cmd(ILI9486_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Interface Pixel Format (16-bit RGB565)
    tft_cmd(ILI9486_PIXFMT);
    tft_data(0x55);  // 16 bits/pixel
    
    // Power Control 1
    tft_cmd(ILI9486_PWCTR1);
    tft_data(0x19);  // VRH[5:0]
    tft_data(0x1A);  // VC[2:0]
    
    // Power Control 2
    tft_cmd(ILI9486_PWCTR2);
    tft_data(0x45);  // SAP[2:0]; BT[3:0]
    tft_data(0x00);
    
    // Power Control 3 (Normal Mode)
    tft_cmd(ILI9486_PWCTR3);
    tft_data(0x33);
    
    // VCOM Control
    tft_cmd(ILI9486_VMCTR1);
    tft_data(0x00);
    tft_data(0x12);
    tft_data(0x80);
    
    // Memory Access Control - LANDSCAPE with BGR
    tft_cmd(ILI9486_MADCTL);
    tft_data(0x28);  // MV=1, BGR=1 for landscape
    
    // Display Function Control
    tft_cmd(ILI9486_DFUNCTR);
    tft_data(0x00);
    tft_data(0x02);  // Non-display area normal
    tft_data(0x3B);  // 480 lines
    
    // Frame Rate Control (Normal Mode)
    tft_cmd(ILI9486_FRMCTR1);
    tft_data(0xB0);  // Division ratio
    tft_data(0x11);  // Frame rate
    
    // Display Inversion Control
    tft_cmd(ILI9486_INVCTR);
    tft_data(0x02);  // 2-dot inversion
    
    // Positive Gamma Control
    tft_cmd(ILI9486_GMCTRP1);
    tft_data(0x0F);
    tft_data(0x24);
    tft_data(0x1C);
    tft_data(0x0A);
    tft_data(0x0F);
    tft_data(0x08);
    tft_data(0x43);
    tft_data(0x88);
    tft_data(0x32);
    tft_data(0x0F);
    tft_data(0x10);
    tft_data(0x06);
    tft_data(0x0F);
    tft_data(0x07);
    tft_data(0x00);
    
    // Negative Gamma Control
    tft_cmd(ILI9486_GMCTRN1);
    tft_data(0x0F);
    tft_data(0x38);
    tft_data(0x30);
    tft_data(0x09);
    tft_data(0x0F);
    tft_data(0x0F);
    tft_data(0x4E);
    tft_data(0x77);
    tft_data(0x3C);
    tft_data(0x07);
    tft_data(0x10);
    tft_data(0x05);
    tft_data(0x23);
    tft_data(0x1B);
    tft_data(0x00);
    
    // Sleep Out (again for safety)
    tft_cmd(ILI9486_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Display ON
    tft_cmd(ILI9486_DISPON);
    vTaskDelay(pdMS_TO_TICKS(25));
    
    // Normal Display Mode ON
    tft_cmd(ILI9486_NORON);
    
    ESP_LOGI(TAG, "Display initialization complete!");
}

// ===== Drawing functions =====

void tft_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Column address set
    tft_cmd(ILI9486_CASET);
    tft_data(x0 >> 8);
    tft_data(x0 & 0xFF);
    tft_data(x1 >> 8);
    tft_data(x1 & 0xFF);

    // Row address set
    tft_cmd(ILI9486_PASET);
    tft_data(y0 >> 8);
    tft_data(y0 & 0xFF);
    tft_data(y1 >> 8);
    tft_data(y1 & 0xFF);

    // Write to RAM
    tft_cmd(ILI9486_RAMWR);
}

void tft_fill_screen(uint16_t color)
{
    ESP_LOGI(TAG, "Filling screen with color 0x%04X", color);
    tft_set_addr_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);

    // Create line buffer
    uint8_t line[TFT_WIDTH * 2];
    for (int i = 0; i < TFT_WIDTH; i++) {
        line[i * 2] = color >> 8;
        line[i * 2 + 1] = color & 0xFF;
    }

    // Set DC high for data transmission
    gpio_set_level(TFT_DC, 1);
    
    // Draw line by line
    for (int y = 0; y < TFT_HEIGHT; y++) {
        tft_data_buf(line, TFT_WIDTH * 2);
        
        // Log progress every 50 lines
        if (y % 50 == 0) {
            ESP_LOGI(TAG, "Drawing line %d/%d", y, TFT_HEIGHT);
        }
    }
    
    ESP_LOGI(TAG, "Screen fill complete");
}

void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT || w <= 0 || h <= 0) return;
    if (x + w > TFT_WIDTH) w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    tft_set_addr_window(x, y, x + w - 1, y + h - 1);

    uint8_t line[w * 2];
    for (int i = 0; i < w; i++) {
        line[i * 2] = color >> 8;
        line[i * 2 + 1] = color & 0xFF;
    }

    gpio_set_level(TFT_DC, 1);
    for (int i = 0; i < h; i++) {
        tft_data_buf(line, w * 2);
    }
}

// ===== Test functions =====

void test_simple(void)
{
    ESP_LOGI(TAG, "\n=== SIMPLE COLOR TEST ===");
    
    ESP_LOGI(TAG, "Test 1: RED");
    tft_fill_screen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "Test 2: GREEN");
    tft_fill_screen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "Test 3: BLUE");
    tft_fill_screen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "Test 4: WHITE");
    tft_fill_screen(TFT_WHITE);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    ESP_LOGI(TAG, "Test 5: BLACK");
    tft_fill_screen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(3000));
}

void test_rectangles(void)
{
    ESP_LOGI(TAG, "\n=== RECTANGLE TEST ===");
    
    tft_fill_screen(TFT_BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    tft_fill_rect(50, 50, 100, 100, TFT_RED);
    tft_fill_rect(200, 50, 100, 100, TFT_GREEN);
    tft_fill_rect(350, 50, 100, 100, TFT_BLUE);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
}

// ===== Main =====

void app_main(void)
{
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "   ILI9486 LCD Test - ESP-IDF");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Hardware Configuration:");
    ESP_LOGI(TAG, "  MOSI (LCD_SI): GPIO%d", TFT_MOSI);
    ESP_LOGI(TAG, "  SCLK (LCD_SCK): GPIO%d", TFT_SCLK);
    ESP_LOGI(TAG, "  CS (LCD_CS):   GPIO%d", TFT_CS);
    ESP_LOGI(TAG, "  DC (LCD_RS):   GPIO%d", TFT_DC);
    ESP_LOGI(TAG, "  RST:           GPIO%d", TFT_RST);
    ESP_LOGI(TAG, "  Display Size:  %dx%d (Landscape)", TFT_WIDTH, TFT_HEIGHT);
    ESP_LOGI(TAG, "========================================\n");

    // Initialize hardware step by step
    ESP_LOGI(TAG, "Step 1: Configuring GPIO pins...");
    tft_init_pins();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Step 2: Initializing SPI bus...");
    tft_init_spi();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Step 3: Hardware reset...");
    tft_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Step 4: Display initialization...");
    tft_init_display();
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "Starting display tests...");
    ESP_LOGI(TAG, "========================================\n");

    // Run tests
    while (1) {
        test_simple();
        test_rectangles();
        
        ESP_LOGI(TAG, "\n========================================");
        ESP_LOGI(TAG, "Test cycle complete. Restarting in 3s...");
        ESP_LOGI(TAG, "========================================\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
