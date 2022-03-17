# Simple SPI display controller

Provides simple tools to allow a screen buffer to be updated with text, boxes, etc. Simple fonts in various sizes.

Handles basic RGB for OLED RGB display, and simple mono for epaper. Updates actual display on background task.

Designed to allow for various SPI based display devices to be selected as part of config.

Note that the current state, graphics buffer, etc, are global, so only one display is supported. This is mainly because the config settings for display size, etc, are all global.
