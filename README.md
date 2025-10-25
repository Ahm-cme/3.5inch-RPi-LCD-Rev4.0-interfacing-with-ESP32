# 3.5inch-RPi-LCD-Rev4.0-interfacing-with-ESP32

ESP32-based LCD display system using an ILI9486 480x320 TFT display designed for Raspberry Pi. This project provides a complete interface implementation for controlling the 3.5" TFT LCD using ESP-IDF framework.

## Hardware Specifications

- **Display Controller**: ILI9486
- **Resolution**: 480x320 pixels
- **Color Format**: RGB565 (16-bit)
- **Interface**: SPI
- **Orientation**: Landscape mode
- **Microcontroller**: ESP32

## Pin Configuration

| LCD Pin | ESP32 GPIO | Function |
|---------|------------|----------|
| LCD_SI  | GPIO 23    | MOSI (Data) |
| LCD_SCK | GPIO 18    | SCLK (Clock) |
| LCD_CS  | GPIO 5     | Chip Select |
| LCD_RS  | GPIO 21    | DC (Data/Command) |
| LCD_RST | GPIO 4     | Reset |
| VCC     | 3.3V       | Power Supply |
| GND     | GND        | Ground |

## Features

- **Complete ILI9486 Driver**: Full initialization sequence with gamma correction
- **SPI Communication**: High-speed data transfer at 10 MHz
- **Color Support**: RGB565 color format with predefined color constants
- **Drawing Functions**:
  - Fill entire screen with solid color
  - Draw filled rectangles
  - Set address windows for pixel-level control
- **Test Suite**: Built-in color tests and pattern demonstrations
- **Hardware Abstraction**: Clean separation between hardware control and display functions

## Project Structure

```
├── CMakeLists.txt          # Main project configuration
├── README.md              # This file
└── main/
    ├── CMakeLists.txt     # Component configuration
    └── main.c             # Main application code
```

## Building and Flashing

### Prerequisites
- ESP-IDF framework (v4.4 or later)
- ESP32 development board
- 3.5" ILI9486 TFT LCD display

### Build Instructions

1. **Set up ESP-IDF environment**:
   ```bash
   source $IDF_PATH/export.sh  # Linux/Mac
   # or
   %IDF_PATH%\export.bat       # Windows
   ```

2. **Configure the project**:
   ```bash
   idf.py set-target esp32
   idf.py menuconfig
   ```

3. **Build the project**:
   ```bash
   idf.py build
   ```

4. **Flash to ESP32**:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor  # Linux
   # or
   idf.py -p COM3 flash monitor          # Windows
   ```

## Code Structure

### Main Components

- **Hardware Initialization**:
  - [`tft_init_pins()`](main/main.c): Configure GPIO pins
  - [`tft_init_spi()`](main/main.c): Initialize SPI bus
  - [`tft_reset()`](main/main.c): Hardware reset sequence

- **Display Control**:
  - [`tft_init_display()`](main/main.c): Complete ILI9486 initialization
  - [`tft_cmd()`](main/main.c): Send command to display
  - [`tft_data()`](main/main.c): Send data to display

- **Drawing Functions**:
  - [`tft_fill_screen()`](main/main.c): Fill entire screen with color
  - [`tft_fill_rect()`](main/main.c): Draw filled rectangles
  - [`tft_set_addr_window()`](main/main.c): Set drawing area

- **Test Functions**:
  - [`test_simple()`](main/main.c): Basic color tests
  - [`test_rectangles()`](main/main.c): Rectangle drawing demo

### Color Constants

The project includes predefined RGB565 colors:
- `TFT_BLACK`, `TFT_WHITE`
- `TFT_RED`, `TFT_GREEN`, `TFT_BLUE`
- `TFT_CYAN`, `TFT_MAGENTA`, `TFT_YELLOW`

## Display Initialization Sequence

The ILI9486 initialization includes:
1. Software reset
2. Interface mode control
3. Sleep out command
4. Pixel format configuration (RGB565)
5. Power control settings
6. VCOM control
7. Memory access control (landscape orientation)
8. Display function control
9. Frame rate control
10. Gamma correction settings
11. Display enable

## Usage Example

```c
// Initialize the display
tft_init_pins();
tft_init_spi();
tft_reset();
tft_init_display();

// Fill screen with red color
tft_fill_screen(TFT_RED);

// Draw a blue rectangle
tft_fill_rect(100, 100, 200, 150, TFT_BLUE);
```

## Troubleshooting

### Common Issues

1. **Blank/White Screen**:
   - Check wiring connections
   - Verify power supply (3.3V)
   - Ensure correct pin assignments

2. **Garbled Display**:
   - Check SPI clock speed (try reducing from 10MHz)
   - Verify MOSI and SCLK connections
   - Check ground connections

3. **No Response**:
   - Verify CS and DC pin connections
   - Check reset pin functionality
   - Monitor serial output for error messages

### Debug Tips

- Enable ESP-IDF logging to monitor initialization steps
- Use oscilloscope to verify SPI signals
- Check power consumption during operation
- Verify LCD compatibility with ILI9486 controller

## Future Enhancements

- Text rendering support
- Bitmap image display
- Touch screen integration
- Advanced graphics primitives (lines, circles)
- Frame buffer implementation
- DMA optimization for faster transfers

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.
