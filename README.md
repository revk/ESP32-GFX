# Simple SPI display controller

Provides simple tools to allow a screen buffer to be updated with text, boxes, etc.

Handles basic RGB for OLED/LCD RGB display, and simple mono for epaper.

Designed to allow for various SPI based display devices to be selected as part of config.

Note that the current state, graphics buffer, etc, are global, so only one display is supported.

- Includes scaled vector based font based on SAA5050 5x9 (Teletext/MODE7) with anti-aliasing.
- Includes scaled 7 segment (plus `.` and `:`), with anti-aliasing
- Includes simple box and line drawing

