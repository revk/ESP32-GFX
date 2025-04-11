# Simple SPI display controller

Provides simple tools to allow a screen buffer to be updated with text, boxes, etc.

Handles basic RGB for OLED/LCD RGB display, and simple mono for epaper.

Designed to allow for various SPI based display devices to be selected as part of config.

Note that the current state, graphics buffer, etc, are global, so only one display is supported.

- Includes scaled vector based font based on SAA5050 5x9 (Teletext/MODE7) with anti-aliasing.
- Includes scaled 7 segment (plus `.` and `:`), with anti-aliasing
- Includes simple box and line drawing

## Display types

### Solid colour (e-paper)

Displays with simple solid colour, such as black/white, black/red/white, etc, are handled quite simply with each pixel being one of the solid colours. The colour picked simply mapper from the RGB specified.

Anti-aliasing and alpha blending are not supported (top bit of alpha determines if plotted of not). As such italics is not recommended (text and 7 segment).

### Greyscale and colour

Displays with greyscale or colours allow antialiasing and alpha blending. A pixel can be set based on an RGB value - for greyscale this is mapped to a grey level.

Alpha blending is handled, mixing the new plotted pixel with the existing based on an alpha level.

Antialiasing is used for text, and 7 segment digits to improve quality, this will be added to other functions (line, circle, etc).

### Key functions

#### `gfx_init`

Sets up the display and options including all GPIOs and a number of flags.

#### `gfx_width` / `gfx_height` / `gfx_bpp`

Provide details of current display size and attriblues

#### `gfx_lock` / `gfx_unlock`

Cann lock, then plot things, then unlock. Depending on the initial settings the display will be updated on unlock or on a background task (which locks while sending).

#### `gfx_foreground`/`gfx_background`

These set the current foreground and background to use. For text and 7 segment a solid background is set as a box first and then the text or 7 segment is plotted. This can be disabled by setting the same colour for foreground and background allowing plotting over the existing pixels with alpha blending. You can check current values with `gfx_f` and `gfx_b`.

#### `gfx_pixel_argb`

This set a pixel based on a 32 bit value (AARRGGBB), an alpha of 0xFF is plot, 0x00 is don't plot, otherwise alpha blend (non antialiasing displays simply test top bit of alpha to plot or not plot).

- `gfx_pixel` plots using current foreground colour and specified  alpha
- `gfx_pixel_rgb` plots specified colour (RRGGBB), same as alpha 0xFF
- `gfx_clear` plots a blend of current background and current foreground for whole display based on an alpha value

#### `gfx_pos`

Sets current position and alignment for other functions. You can get current values `gf_x`, `gfx_y`, `gfx_a`.

#### `gfx_text` and `gfx_7seg`

These plot text or fixed digits. They have a number of flags and a based size. It plots at current position and alignment and moves to new position.

- `gfx_text` plots vector based text including a load of unicode characters - this allows newlines
- `gfx_7seg` plots digits, and `.` or `:` after a digit, and a number of letters than can be expressed using 7 segment display format

You can check size of text with `gfx_text_size` and `gfx_7seg_size`.

#### `gfx_line`

A number of extra functions exist - these are being updated and new functions added.

### Some simple data tables

- `sin256` a table of sin() for 90 degrees (0-255) giving value 0-255
- `circle256` a table of x for y for a quarter circle 0-255 giving value 0-255

### Images

You can make use of the `lwpng` library for processing a png and calling `gfx_pixel_argb`. If you want to access the frame buffer `gfx_raw_w`, `gfx_raw_h` and `gfx_raw_b` allow access to the raw buffer - you will need to decode pixel values depending on `gfx_bpp` if you want to make a png.
