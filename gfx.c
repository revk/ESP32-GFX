/*
 * Simple display and text logic Copyright �2019-22 Adrian Kennard, Andrews & Arnold Ltd
 * 
 * The drawing functions all text and some basic graphics Always use gfx_lock() and gfx_unlock() around drawing, this ensures they
 * are atomically updated to the physical display, and allows the drawing state to be held without clashes with other tasks.
 * 
 * The drawing state includes:- - Position of cursor - Foreground and background colour - Alignment of that position in next drawn
 * object - Movement after drawing (horizontal or vertical)
 * 
 * Pixels are set to an "intensity" (0-255) to which a current foreground and background colour is applied. In practice this may be
 * fewer bits, e.g. on SDD1351 only top 4 bits of intensity are used, multiplied by the selected colour to make a 16 bit RGB For a
 * mono display the intensity directly relates to the grey scale used.
 * 
 * Functions are described in the include file.
 * 
 */

#define	UNUSED __attribute__((unused))
static UNUSED const char TAG[] = "GFX";
#define	DEBUG	ESP_LOGD

#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <hal/spi_types.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "gfx.h"
#ifdef	  CONFIG_REVK_APPNAME
#include "../../main/settings.h"
#endif

#define	SPI_MAX	16384

#ifdef	CONFIG_GFX_BUILD_SUFFIX_GFXNONE
const char *
gfx_init_opts (gfx_init_t o)
{                               // Dummy - no driver
   return "Not configured";
}

void
gfx_lock (void)
{                               // Dummy - no driver
}

void
gfx_unlock (void)
{                               // Dummy - no driver
}

void
gfx_set_contrast (gfx_intensity_t i)
{                               // Dummy - no driver
}

void
gfx_pos (gfx_pos_t x, gfx_pos_t y, gfx_align_t a)
{                               // Dummy - no driver
}

void
gfx_colour (char c)
{                               // Dummy - no driver
}

void
gfx_background (char c)
{                               // Dummy - no driver
}

uint16_t
gfx_width (void)
{                               // Dummy - no driver
   return 0;
}

uint16_t
gfx_height (void)
{                               // Dummy - no driver
   return 0;
}

uint8_t
gfx_bpp (void)
{                               // Dummy - no driver
   return 0;
}

gfx_pos_t
gfx_x (void)
{                               // Dummy - no driver
   return 0;
}

gfx_pos_t
gfx_y (void)
{                               // Dummy - no driver
   return 0;
}

gfx_align_t
gfx_a (void)
{                               // Dummy - no driver
   return 0;
}

char
gfx_f (void)
{                               // Dummy - no driver
   return 0;
}

char
gfx_b (void)
{                               // Dummy - no driver
   return 0;
}

void
gfx_pixel (gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i)
{                               // Dummy - no driver
}

void
gfx_clear (gfx_intensity_t i)
{                               // Dummy - no driver
}

void
gfx_box (gfx_pos_t w, gfx_pos_t h, gfx_intensity_t i)
{                               // Dummy - no driver
}

void
gfx_fill (gfx_pos_t w, gfx_pos_t h, gfx_intensity_t i)
{                               // Dummy - no driver
}

void
gfx_text (uint8_t flags, uint8_t size, const char *fmt, ...)
{                               // Dummy - no driver
}

void
gfx_text_size (uint8_t flags, uint8_t size, const char *t, gfx_pos_t * w, gfx_pos_t * h)
{                               // Dummy - no driver
}


void
gfx_7seg (uint8_t flags, int8_t size, const char *fmt, ...)
{                               // Dummy - no driver
}

void
gfx_7seg_size (uint8_t flags, int8_t size, const char *, gfx_pos_t * w, gfx_pos_t * h)
{                               // Dummy - no driver
}

void
gfx_icon16 (gfx_pos_t w, gfx_pos_t h, const void *data)
{                               // Dummy - no driver
}

void
gfx_message (const char *m)
{                               // Dummy - no driver
}

int
gfx_ok (void)
{
   return 0;
}

void
gfx_sleep (void)
{
}

void
gfx_flip (uint8_t flip)
{
}

void
gfx_border (uint8_t border)
{
}

void
gfx_line (gfx_pos_t x1, gfx_pos_t y1, gfx_pos_t x2, gfx_pos_t y2, gfx_intensity_t l)
{                               // Draw a line
}

uint16_t
gfx_raw_w (void)
{
   return 0;
}

uint16_t
gfx_raw_h (void)
{
   return 0;
}

void *
gfx_raw_b (void)
{
   return NULL;
}
#else

// general global stuff
static gfx_init_t gfx_settings = { };

static TaskHandle_t gfx_task_id = NULL;
static SemaphoreHandle_t gfx_mutex = NULL;
static spi_device_handle_t gfx_spi;

// Driver support
static UNUSED void gfx_busy_wait (void);        // Manual wait if no busy set
static esp_err_t gfx_send_gfx (uint8_t);
static esp_err_t gfx_send_data (const void *data, uint32_t len);
static esp_err_t gfx_command (uint8_t cmd, const uint8_t * buf, uint8_t len);
static UNUSED esp_err_t gfx_command0 (uint8_t cmd);
static UNUSED esp_err_t gfx_command1 (uint8_t cmd, uint8_t a);
static UNUSED esp_err_t gfx_command2 (uint8_t cmd, uint8_t a, uint8_t b);
static UNUSED esp_err_t gfx_command4 (uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
static UNUSED esp_err_t gfx_command_bulk (const uint8_t * init_code);

// Driver (and defaults for driver)
#ifdef  CONFIG_GFX_BUILD_SUFFIX_SSD1351
#include "ssd1351.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_SSD1680
#include "ssd1680.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_SSD1681
#include "ssd1681.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_EPD75K
#include "epd75k.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_EPD75R
#include "epd75r.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_EPD154K
#include "epd154k.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_EPD154R
#include "epd154r.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_EPD29K
#include "epd29k.c"
#endif
#ifdef  CONFIG_GFX_BUILD_SUFFIX_LCD24
#include "lcd24.c"
#endif

#ifdef	CONFIG_GFX_UNICODE
#include "vector.h"
#else
#include "vector96.h"
#endif

#ifdef	CONFIG_GFX_7SEG
#include "7seg.h"
#endif

static char const sevensegchar[] = " 0123456789-_\"',[]ABCDEFGHIJLNOPRSUZ";
static uint8_t const sevensegmap[] = {
   0x00,                        // space
   0x3F,                        // 0
   0x06,                        // 1
   0x5B,                        // 2
   0x4F,                        // 3
   0x66,                        // 4
   0x6D,                        // 5
   0x7D,                        // 6
   0x07,                        // 7
   0x7F,                        // 8
   0x6F,                        // 9
   0x40,                        // -
   0x08,                        // _
   0x22,                        // "
   0x02,                        // '
   0x04,                        // ,
   0x39,                        // [
   0x0F,                        // ]
   0x77,                        // A
   0x7C,                        // B (b)
   0x39,                        // C
   0x5E,                        // D (d)
   0x79,                        // E
   0x71,                        // F
   0x3D,                        // G
   0x76,                        // H
   0x30,                        // I
   0x1E,                        // J
   0x38,                        // L
   0x37,                        // N
   0x3F,                        // O
   0x73,                        // P
   0x50,                        // R (r)
   0x6D,                        // S
   0x3E,                        // U
   0x5B,                        // Z
};

#define	BLACK	0

#if GFX_BPP == 16               // 16 bit RGB
#define GFX_INTENSITY_BPP  4    // We work on each colour being 4 bits intensity based on one of a set of colours
#define R       (1<<11)
#define G       (1<<5)
#define B       (1)

#define RED     (R+R)
#define GREEN   (G+G+G+G)
#define BLUE    (B+B)

#define CYAN    (GREEN+BLUE)
#define MAGENTA (RED+BLUE)
#define YELLOW  (RED+GREEN)

#define	black	0
#define WHITE   (RED+GREEN+BLUE)

#elif GFX_BPP == 1              // Mono

#define WHITE   1

#elif GFX_BPP == 2              // Black/red/white

#define WHITE   1
#define	RED	2

#elif GFX_BPP <= 8              // Greyscale or mono

#define WHITE   255
#define GFX_INTENSITY_BPP  GFX_BPP

#endif

#if GFX_BPP > 16
typedef uint32_t gfx_cell_t;
#define GFX_SIZE (gfx_settings.width * gfx_settings.height * sizeof(gfx_cell_t))
#define	GFX_PAGE	GFX_SIZE
#elif GFX_BPP > 8
typedef uint16_t gfx_cell_t;
#define GFX_SIZE (gfx_settings.width * gfx_settings.height * sizeof(gfx_cell_t))
#define	GFX_PAGE	GFX_SIZE
#elif GFX_BPP == 2
typedef uint8_t gfx_cell_t;
#define GFX_PAGE ((gfx_settings.width + 7) / 8 * gfx_settings.height)
#define GFX_SIZE (GFX_PAGE*2)
#elif GFX_BPP == 1
typedef uint8_t gfx_cell_t;
#define GFX_SIZE ((gfx_settings.width + 7) / 8 * gfx_settings.height)
#define	GFX_PAGE	GFX_SIZE
#else // Grey, etc
typedef uint8_t gfx_cell_t;
#define GFX_SIZE ((gfx_settings.width * GFX_BPP + 7) / 8 * gfx_settings.height)
#define	GFX_PAGE	GFX_SIZE
#endif
static gfx_cell_t *gfx = NULL;

// drawing state
static gfx_pos_t x = 0,
   y = 0;                       // position
static gfx_align_t a = 0;       // alignment and movement
static char f = 0,              // colour
   b = 0;
#if GFX_BPP <= 8
static uint8_t f_mul = 0;
static uint8_t b_mul = 0;       // actual f/b colour multiplier
#else
static uint32_t f_mul = 0;
static uint32_t b_mul = 0;      // actual f/b colour multiplier
#endif

// Driver support

static void
gfx_busy_wait (void)
{                               // manual wait if BUSY not set
   if (!gfx_settings.busy)
      sleep (5);
   else
   {
      uint16_t try = 3000;
#ifdef	GFX_BUSY_LOW
      while (--try && !gpio_get_level (gfx_settings.busy))
#else
      while (--try && gpio_get_level (gfx_settings.busy))
#endif
         usleep (10000);
      if (!try)
         ESP_LOGE (TAG, "Busy too long");
   }
}

static esp_err_t
gfx_send_command (uint8_t cmd)
{
   if (gfx_settings.busy)
   {                            // Check busy
      uint16_t try = 30000;
#ifdef	GFX_BUSY_LOW
      while (--try && !gpio_get_level (gfx_settings.busy))
#else
      while (--try && gpio_get_level (gfx_settings.busy))
#endif
         usleep (10000);
      if (!try)
         ESP_LOGE (TAG, "Busy before command %02X", cmd);
   }
   gpio_set_level (gfx_settings.dc, 0);
   spi_transaction_t t = {
      .length = 8,
      .tx_data = {cmd},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   esp_err_t e = spi_device_transmit (gfx_spi, &t);
   return e;
}

void
gfx_load (const void *data)
{
   if (!data)
      return;
   memcpy (gfx, data, GFX_SIZE);
   gfx_settings.changed = 1;
}

static esp_err_t
gfx_send_data (const void *data, uint32_t len)
{
   gpio_set_level (gfx_settings.dc, 1);
   while (len)
   {
      uint32_t l = len;
      if (l > SPI_MAX)
         l = SPI_MAX;
      spi_transaction_t c = {
         .length = 8 * l,
         .tx_buffer = data,
      };
      esp_err_t e = spi_device_transmit (gfx_spi, &c);
      if (e)
      {
         ESP_LOGE (TAG, "Failed send data (%lu)", l);
         return e;
      }
      len -= l;
      data += l;
   }
   return 0;
}

static esp_err_t
gfx_send_gfx (uint8_t page)
{
   DEBUG (TAG, "Data page %d: %d", page, GFX_PAGE);
   return gfx_send_data (gfx + page * GFX_PAGE, GFX_PAGE);
}

static esp_err_t
gfx_command (uint8_t cmd, const uint8_t * buf, uint8_t len)
{
   DEBUG (TAG, "Command %02X+%d", cmd, len);
   esp_err_t e = gfx_send_command (cmd);
   if (!e && len)
      e = gfx_send_data (buf, len);
   return e;
}

static UNUSED esp_err_t
gfx_command0 (uint8_t cmd)
{
   DEBUG (TAG, "Command %02X", cmd);
   return gfx_send_command (cmd);
}

static UNUSED esp_err_t
gfx_command1 (uint8_t cmd, uint8_t a)
{                               // Send a command with an arg
   DEBUG (TAG, "Command %02X %02X", cmd, a);
   esp_err_t e = gfx_send_command (cmd);
   if (e)
      return e;
   gpio_set_level (gfx_settings.dc, 1);
   spi_transaction_t t = {
      .length = 8,
      .tx_data = {a},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_transmit (gfx_spi, &t);
}

static UNUSED esp_err_t
gfx_command2 (uint8_t cmd, uint8_t a, uint8_t b)
{                               // Send a command with args
   DEBUG (TAG, "Command %02X %02X %02X", cmd, a, b);
   esp_err_t e = gfx_send_command (cmd);
   if (e)
      return e;
   gpio_set_level (gfx_settings.dc, 1);
   spi_transaction_t t = {
      .length = 16,
      .tx_data = {a, b},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_transmit (gfx_spi, &t);
}

static UNUSED esp_err_t
gfx_command4 (uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{                               // Send a command with args
   DEBUG (TAG, "Command %02X %02X %02X %02X %02X", cmd, a, b, c, d);
   esp_err_t e = gfx_send_command (cmd);
   if (e)
      return e;
   gpio_set_level (gfx_settings.dc, 1);
   spi_transaction_t t = {
      .length = 32,
      .tx_data = {a, b, c, d},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_transmit (gfx_spi, &t);
}

static UNUSED esp_err_t
gfx_command_bulk (const uint8_t * bulk)
{                               // Bulk command
   // Bulk is a sequence of blocks of the form :-
   // Len (0 for end) 0xFF is delay
   // Command (included in len)
   // Data (len-1 bytes)

   while (*bulk)
   {
      uint8_t len = *bulk++;
      if (len == 0xFF)
      {
         ESP_LOGD (TAG, "Pause");
         usleep (100000);
         continue;
      }
      esp_err_t e = gfx_command (*bulk, bulk + 1, len - 1);
      if (e)
         return e;
      bulk += len;
   }
   return 0;
}

// state control
void
gfx_pos (gfx_pos_t newx, gfx_pos_t newy, gfx_align_t newa)
{                               // Set position
   x = newx;
   y = newy;
   a = (newa ? : (GFX_L | GFX_T | GFX_H));
}

static uint32_t
gfx_colour_lookup (char c)
{                               // character to colour mapping, default is white
   switch (c)
   {
   case 'k':
   case 'K':
      return BLACK;
#if GFX_BPP ==2
   case 'R':
      return RED;
#elif GFX_BPP > 8
   case 'r':
      return (RED >> 1);
   case 'R':
      return RED;
   case 'g':
      return (GREEN >> 1);
   case 'G':
      return GREEN;
   case 'b':
      return (BLUE >> 1);
   case 'B':
      return BLUE;
   case 'c':
      return (CYAN >> 1);
   case 'C':
      return CYAN;
   case 'm':
      return (MAGENTA >> 1);
   case 'M':
      return MAGENTA;
   case 'y':
      return (YELLOW >> 1);
   case 'Y':
      return YELLOW;
   case 'w':
      return (WHITE >> 1);
   case 'o':
   case 'O':
      return RED + (GREEN >> 1);
#endif
   }
   return WHITE;
}

void
gfx_colour (char newf)
{                               // Set foreground
   f_mul = gfx_colour_lookup (f = newf);
}

void
gfx_background (char newb)
{                               // Set background
   b_mul = gfx_colour_lookup (b = newb);
}

// Basic settings
uint16_t
gfx_width (void)
{                               // Display width
   return (gfx_settings.flip & 4) ? gfx_settings.height : gfx_settings.width;
}

uint16_t
gfx_height (void)
{                               // Display height
   return (gfx_settings.flip & 4) ? gfx_settings.width : gfx_settings.height;
}

uint8_t
gfx_bpp (void)
{
   return GFX_BPP;
}

// State get
gfx_pos_t
gfx_x (void)
{
   return x;
}

gfx_pos_t
gfx_y (void)
{
   return y;
}

gfx_align_t
gfx_a (void)
{
   return a;
}

char
gfx_f (void)
{
   return f;
}

char
gfx_b (void)
{
   return b;
}

// support
inline void
gfx_pixel (gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i)
{                               // set a pixel
   if (!gfx)
      return;
   if (gfx_settings.flip & 4)
   {
      gfx_pos_t t = x;
      x = y;
      y = t;
   };
   if (gfx_settings.flip & 1)
      x = gfx_settings.width - 1 - x;
   if (gfx_settings.flip & 2)
      y = gfx_settings.height - 1 - y;
   if (x < 0 || x >= gfx_settings.width || y < 0 || y >= gfx_settings.height)
      return;                   // out of display
#if GFX_BPP > 2
   if (gfx_settings.contrast < 4)
      i >>= (4 - gfx_settings.contrast + ((x ^ y) & 1));        // Extra dim and dithered
   else if (gfx_settings.contrast < 4)
      i >>= (8 - gfx_settings.contrast);        // Extra dim
#endif
   if (i < 128 && f == b)
      return;                   // Mask mode
#if GFX_BPP == 1                // Black/white
   const int shift = 7 - (x % 8);
   const int line = (gfx_settings.width + 7) / 8;
   const int addr = line * y + x / 8;
   if (i >= 85 && i < 170)
      i = (((x + y) & 1) ? 255 : 0);    // low level dither
   uint8_t k = ((i & 0x80) ? f_mul : b_mul) & 1;
   if (gfx_settings.invert)
      k ^= 1;
   if (((gfx[addr] >> shift) & 1) != k)
   {
      gfx[addr] = ((gfx[addr] & ~(1 << shift)) | (k << shift));
      gfx_settings.changed = 1;
   }
#elif GFX_BPP == 2              // Black/red/white
   const int shift = 7 - (x % 8);
   const int line = (gfx_settings.width + 7) / 8;
   const int addr = line * y + x / 8;
   uint8_t k = ((i & 0x80) ? f_mul : b_mul) & 1;
   if (gfx_settings.invert)
      k ^= 1;
   if (((gfx[addr] >> shift) & 1) != k)
   {
      gfx[addr] = ((gfx[addr] & ~(1 << shift)) | (k << shift));
      gfx_settings.changed = 1;
   }
   uint8_t r = (((i & 0x80) ? f_mul : b_mul) >> 1) & 1;
   if (((gfx[GFX_PAGE + addr] >> shift) & 1) != r)
   {
      gfx[GFX_PAGE + addr] = ((gfx[GFX_PAGE + addr] & ~(1 << shift)) | (r << shift));
      gfx_settings.changed = 1;
   }
#elif GFX_BPP <= 8              // Grey
   const int bits = (1 << GFX_BPP) - 1;
   const int shift = 8 - (x % (8 / GFX_BPP)) - GFX_BPP;
   const int line = (gfx_settings.width * GFX_BPP + 7) / 8;
   const int addr = line * y + x * GFX_BPP / 8;
   i >>= (8 - GFX_BPP);
   i &= bits;
   if (gfx_settings.invert)
      i ^= bits;
   if (((gfx[addr] >> shift) & bits) != i)
   {
      gfx[addr] = ((gfx[addr] & ~(bits << shift)) | (i << shift));
      gfx_settings.changed = 1;
   }
#else // Colour (ignore invert)
   uint16_t v = ntohs (f_mul * (i >> (8 - GFX_INTENSITY_BPP)) + b_mul * ((0xFF ^ i) >> (8 - GFX_INTENSITY_BPP)));
   if (v != gfx[(y * gfx_settings.width) + x])
   {
      gfx[(y * gfx_settings.width) + x] = v;
      gfx_settings.changed = 1;
   }
#endif
}

static void
gfx_pixel0 (gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i)
{
   gfx_pixel (x, y, 0);
}

void
gfx_draw (gfx_pos_t w, gfx_pos_t h, gfx_pos_t wm, gfx_pos_t hm, gfx_pos_t * xp, gfx_pos_t * yp)
{                               // move x/y based on drawing a box w/h, set x/y as top left of said box
   if (!gfx)
      return;
   gfx_pos_t l = x,
      t = y;
   if ((a & GFX_C) == GFX_C)
      l -= (w - 1) / 2;
   else if (a & GFX_R)
      l -= (w - 1);
   if ((a & GFX_M) == GFX_M)
      t -= (h - 1) / 2;
   else if (a & GFX_B)
      t -= (h - 1);
   if (a & GFX_H)
   {
      if (a & GFX_L)
         x += w + wm;
      if (a & GFX_R)
         x -= w + wm;
   }
   if (a & GFX_V)
   {
      if (a & GFX_T)
         y += h + hm;
      if (a & GFX_B)
         y -= h + hm;
   }
   if (xp)
      *xp = l;
   if (yp)
      *yp = t;
}

const uint8_t *
gfx_pack (const uint8_t * data, uint8_t * lx, uint8_t * hx, uint8_t * ly, uint8_t * hy, uint8_t ppb)
{                               // Pack range bytes
   if (!data)
   {                            // No range
      *lx = 1;
      *hx = 0;
      *ly = 1;
      *hy = 0;
      return data;
   }
   *lx = ((data[0] >> 5) + ((data[2] & 0x80) >> 4)) * ppb;
   *hx = *lx + ((data[0] & 0x1F) + 1) * ppb;
   *ly = data[1];
   *hy = *ly + (data[2] & 0x7F) + 1;
   return data + 3;
}

static UNUSED void
gfx_block2N_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, uint8_t mx, uint8_t my, const uint8_t * data)
{                               // Draw a block from 2 bit image data, c is colour to plot where icon is black/set
   uint8_t lx,
     hx,
     ly,
     hy;
   data = gfx_pack (data, &lx, &hx, &ly, &hy, 8);
   uint8_t d = 0;
   for (gfx_pos_t row = 0; row < h; row++)
   {
      for (gfx_pos_t col = 0; col < w; col++)
      {
         if (row >= ly && row < hy && col >= lx && col < hx && !(col & 7))
            d = *data++;
         if (col >= dx)
            for (uint8_t qx = 0; qx < mx; qx++)
               for (uint8_t qy = 0; qy < mx; qy++)
                  gfx_pixel (x + (col - dx) * mx + qx, y + row * my + qy, (d & 0x80) ? 255 : 0);
         d <<= 1;
      }
   }
}

static UNUSED void
gfx_mask_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t dx, const uint8_t * data, gfx_intensity_t i)
{                               // Draw a block from 2 bit image data, c is colour to plot where icon is black/set - data is packed
   uint8_t lx,
     hx,
     ly,
     hy;
   data = gfx_pack (data, &lx, &hx, &ly, &hy, 8);
   uint8_t d = 0;
   for (gfx_pos_t row = ly; row < hy; row++)
      for (gfx_pos_t col = lx; col < hx; col++)
      {
         if (!(col & 7))
            d = *data++;
         if ((d & 0x80) && col >= dx)
            gfx_pixel (x + col - dx, y + row, i);
         d <<= 1;
      }
}

static UNUSED void
gfx_block2 (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data, int l)
{                               // Draw a block from 2 bit image data, l is data width for each row, c is colour to plot where icon is black/set
   if (!l)
      l = (w + 7) / 8;          // default is pixels width
   for (gfx_pos_t row = 0; row < h; row++)
   {
      for (gfx_pos_t col = 0; col < w; col++)
         gfx_pixel (x + col, y + row, ((data[(col + dx) / 8] >> ((col + dx) & 7)) & 1) ? 255 : 0);
      data += l;
   }
}

static UNUSED void
gfx_block2_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data)
{                               // Draw a block from 2 bit image data, c is colour to plot where icon is black/set
   uint8_t lx,
     hx,
     ly,
     hy;
   data = gfx_pack (data, &lx, &hx, &ly, &hy, 8);
   uint8_t d = 0;
   w += dx;
   for (gfx_pos_t row = 0; row < h; row++)
      for (gfx_pos_t col = 0; col < w || col < hx; col++)
      {
         if (row >= ly && row < hy && col >= lx && col < hx && !(col & 7))
            d = *data++;
         if (col >= dx && col < w)
            gfx_pixel (x + col - dx, y + row, (d & 0x80) ? 255 : 0);
         d <<= 1;
      }
}

static UNUSED void
gfx_block16 (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, const uint8_t * data)
{                               // Draw a block from 16 bit greyscale data, l is data width for each row
   uint8_t d = 0;
   for (gfx_pos_t row = 0; row < h; row++)
      for (gfx_pos_t col = 0; col < w; col++)
      {
         if (!(col & 1))
            d = *data++;
         gfx_pixel (x + col, y + row, (d & 0xF0) | (d >> 4));
         d <<= 4;
      }
}

static UNUSED void
gfx_block16_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data)
{                               // Draw a block from 16 bit greyscale data
   uint8_t lx,
     hx,
     ly,
     hy;
   data = gfx_pack (data, &lx, &hx, &ly, &hy, 2);
   uint8_t d = 0;
   w += dx;
   for (gfx_pos_t row = 0; row < h; row++)
      for (gfx_pos_t col = 0; col < w || col < hx; col++)
      {
         if (row >= ly && row < hy && col >= lx && col < hx && !(col & 1))
            d = *data++;
         if (col >= dx && col < w)
            gfx_pixel (x + col - dx, y + row, (d & 0xF0) | (d >> 4));
         d <<= 4;
      }
}

// drawing
void
gfx_clear (gfx_intensity_t i)
{
   if (!gfx)
      return;
   for (gfx_pos_t y = 0; y < gfx_height (); y++)
      for (gfx_pos_t x = 0; x < gfx_width (); x++)
         gfx_pixel (x, y, i);
}

void
gfx_set_contrast (gfx_intensity_t contrast)
{
   if (!gfx)
      return;
   if (!contrast)
      contrast = 255;
   if (!gfx || gfx_settings.contrast == contrast)
      return;
   gfx_settings.contrast = contrast;
   gfx_settings.update = 1;
   gfx_settings.changed = 1;
}

void
gfx_box (gfx_pos_t w, gfx_pos_t h, gfx_intensity_t i)
{                               // draw a box, not filled
   if (!gfx)
      return;
   gfx_pos_t x,
     y;
   gfx_draw (w, h, 0, 0, &x, &y);
   if (f != b)
      for (gfx_pos_t n = 0; n < w; n++)
      {
         gfx_pixel (x + n, y, i);
         gfx_pixel (x + n, y + h - 1, i);
      }
   for (gfx_pos_t n = 1; n < h - 1; n++)
   {
      gfx_pixel (x, y + n, i);
      gfx_pixel (x + w - 1, y + n, i);
   }
}

void
gfx_fill (gfx_pos_t w, gfx_pos_t h, gfx_intensity_t i)
{                               // draw a filled rectangle
   if (!gfx)
      return;
   gfx_pos_t x,
     y;
   gfx_draw (w, h, 0, 0, &x, &y);
   if (f != b)
      for (gfx_pos_t row = 0; row < h; row++)
         for (gfx_pos_t col = 0; col < w; col++)
            gfx_pixel (x + col, y + row, i);
}

void
gfx_icon2 (gfx_pos_t w, gfx_pos_t h, const void *data)
{                               // Icon, 2 bit packed
   gfx_pos_t x,
     y;
   gfx_draw (w, h, 0, 0, &x, &y);
   if (data)
      gfx_block2 (x, y, w, h, 0, data, 0);
}

void
gfx_icon16 (gfx_pos_t w, gfx_pos_t h, const void *data)
{                               // Icon, 16 bit packed
   if (!gfx)
      return;
   if (!data)
      gfx_fill (w, h, 0);       // No icon
   else
   {
      gfx_pos_t x,
        y;
      gfx_draw (w, h, 0, 0, &x, &y);
      gfx_block16 (x, y, w, h, data);
   }
}

typedef void gfx_pixel_t (gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i);

// Run length plotting

static void
plot_run (gfx_pixel_t * p, gfx_pos_t x, gfx_pos_t y, uint8_t runs, gfx_pos_t * run)
{                               // Simple plot of run lengths (run length is pixels)
   while (runs--)
   {
      gfx_pos_t l = *run++;
      gfx_pos_t r = *run++;
      while (l < r)
         p (x + l++, y, (gfx_intensity_t) - 1);
   }
}


static void
plot_runs (gfx_pixel_t * p, gfx_pos_t x, gfx_pos_t y, uint8_t aa, uint8_t * runs, gfx_pos_t ** run)
{                               // Anti-alias plot of runs aa x aa (run length is pixels*aa)
   if (!aa)
      return;
   if (aa == 1)
   {
      plot_run (p, x, y, *runs, *run);
      return;
   }
   uint8_t pos[aa];
   memset (pos, 0, aa);
   uint8_t sum = 0;
   gfx_pos_t l = 0;
   while (1)
   {
      gfx_pos_t r = -1;         // Next step
      for (uint8_t a = 0; a < aa; a++)
         if (pos[a] < runs[a] * 2 && (r < 0 || run[a][pos[a]] < r))
            r = run[a][pos[a]];
      if (r < 0)
         break;
      uint8_t c = 0;            // Count how many in range
      for (uint8_t a = 0; a < aa; a++)
         if (pos[a] & 1)
            c++;
      while (l < r)
      {
         sum += c;
         l++;
         if (!(l % aa))
         {
            if (sum)
               p (x + l / aa - 1, y, (int16_t) ((gfx_intensity_t) - 1) * sum / aa / aa);
            sum = 0;
         }
      }
      l = r;
      for (uint8_t a = 0; a < aa; a++)
         if (pos[a] < runs[a] * 2 && run[a][pos[a]] <= l)
            pos[a]++;
   }
}

static uint8_t
add_run (gfx_pos_t * run, uint8_t runs, uint8_t max, gfx_pos_t l, gfx_pos_t r)
{                               // Update run with new run l to r, return new run len
   if (runs == max)
      return runs;
   uint8_t n = 0;
   while (n < runs && run[n * 2] < l)
      n++;
   if (n < runs)
      memmove (run + n * 2 + 2, run + n * 2, (runs - n) * 2 * sizeof (*run));
   runs++;
   run[n * 2] = l;
   run[n * 2 + 1] = r;
   void merge (void)
   {                            // merge to next
      while (n + 1 < runs && run[n * 2 + 1] >= run[n * 2 + 2])
      {                         // merge next
         if (run[n * 2 + 3] > run[n * 2 + 1])
            run[n * 2 + 1] = run[n * 2 + 3];
         if (n + 1 < runs)
            memmove (run + n * 2 + 2, run + n * 2 + 4, (runs - n - 1) * 2 * sizeof (*run));
         runs--;
      }
   }
   merge ();
   if (!n)
      return runs;
   n--;
   merge ();
   return runs;
}

const uint8_t circle256[256] = {
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFC, 0xFC, 0xFC,
   0xFC, 0xFC, 0xFC, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8,
   0xF7, 0xF7, 0xF7, 0xF7, 0xF6, 0xF6, 0xF6, 0xF6, 0xF5, 0xF5, 0xF5, 0xF4, 0xF4, 0xF4, 0xF3, 0xF3, 0xF3, 0xF3, 0xF2, 0xF2, 0xF1,
   0xF1, 0xF1, 0xF0, 0xF0, 0xF0, 0xEF, 0xEF, 0xEF, 0xEE, 0xEE, 0xED, 0xED, 0xED, 0xEC, 0xEC, 0xEB, 0xEB, 0xEB, 0xEA, 0xEA, 0xE9,
   0xE9, 0xE8, 0xE8, 0xE7, 0xE7, 0xE6, 0xE6, 0xE5, 0xE5, 0xE4, 0xE4, 0xE3, 0xE3, 0xE2, 0xE2, 0xE1, 0xE1, 0xE0, 0xE0, 0xDF, 0xDF,
   0xDE, 0xDD, 0xDD, 0xDC, 0xDC, 0xDB, 0xDB, 0xDA, 0xD9, 0xD9, 0xD8, 0xD7, 0xD7, 0xD6, 0xD5, 0xD5, 0xD4, 0xD3, 0xD3, 0xD2, 0xD1,
   0xD1, 0xD0, 0xCF, 0xCF, 0xCE, 0xCD, 0xCC, 0xCC, 0xCB, 0xCA, 0xC9, 0xC9, 0xC8, 0xC7, 0xC6, 0xC5, 0xC4, 0xC4, 0xC3, 0xC2, 0xC1,
   0xC0, 0xBF, 0xBE, 0xBE, 0xBD, 0xBC, 0xBB, 0xBA, 0xB9, 0xB8, 0xB7, 0xB6, 0xB5, 0xB4, 0xB3, 0xB2, 0xB1, 0xB0, 0xAF, 0xAE, 0xAD,
   0xAC, 0xAB, 0xA9, 0xA8, 0xA7, 0xA6, 0xA5, 0xA4, 0xA2, 0xA1, 0xA0, 0x9F, 0x9D, 0x9C, 0x9B, 0x99, 0x98, 0x97, 0x95, 0x94, 0x93,
   0x91, 0x90, 0x8E, 0x8D, 0x8B, 0x8A, 0x88, 0x87, 0x85, 0x83, 0x82, 0x80, 0x7E, 0x7C, 0x7B, 0x79, 0x77, 0x75, 0x73, 0x71, 0x6F,
   0x6D, 0x6B, 0x68, 0x66, 0x64, 0x61, 0x5F, 0x5D, 0x5A, 0x57, 0x54, 0x52, 0x4F, 0x4B, 0x48, 0x45, 0x41, 0x3D, 0x39, 0x34, 0x2F,
   0x2A, 0x23, 0x1B, 0x0F
};

const uint8_t sin256[256] = {
   0x00, 0x01, 0x03, 0x04, 0x06, 0x07, 0x09, 0x0A, 0x0C, 0x0E, 0x0F, 0x11, 0x12, 0x14, 0x15, 0x17, 0x19, 0x1A, 0x1C, 0x1D, 0x1F,
   0x20, 0x22, 0x24, 0x25, 0x27, 0x28, 0x2A, 0x2B, 0x2D, 0x2E, 0x30, 0x31, 0x33, 0x35, 0x36, 0x38, 0x39, 0x3B, 0x3C, 0x3E, 0x3F,
   0x41, 0x42, 0x44, 0x45, 0x47, 0x48, 0x4A, 0x4B, 0x4D, 0x4E, 0x50, 0x51, 0x53, 0x54, 0x56, 0x57, 0x59, 0x5A, 0x5C, 0x5D, 0x5F,
   0x60, 0x61, 0x63, 0x64, 0x66, 0x67, 0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x73, 0x74, 0x75, 0x77, 0x78, 0x7A, 0x7B, 0x7C,
   0x7E, 0x7F, 0x80, 0x82, 0x83, 0x84, 0x86, 0x87, 0x88, 0x8A, 0x8B, 0x8C, 0x8E, 0x8F, 0x90, 0x92, 0x93, 0x94, 0x95, 0x97, 0x98,
   0x99, 0x9A, 0x9C, 0x9D, 0x9E, 0x9F, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAE, 0xAF, 0xB0, 0xB1,
   0xB2, 0xB3, 0xB4, 0xB5, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD7, 0xD8, 0xD9, 0xDA,
   0xDB, 0xDC, 0xDC, 0xDD, 0xDE, 0xDF, 0xDF, 0xE0, 0xE1, 0xE2, 0xE2, 0xE3, 0xE4, 0xE4, 0xE5, 0xE6, 0xE6, 0xE7, 0xE8, 0xE8, 0xE9,
   0xEA, 0xEA, 0xEB, 0xEC, 0xEC, 0xED, 0xED, 0xEE, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF1, 0xF2, 0xF2, 0xF3, 0xF3, 0xF3, 0xF4, 0xF4,
   0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB,
   0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
   0xFE, 0xFE, 0xFE, 0xFF
};

static void
plot_5x9 (gfx_pixel_t * p, gfx_pos_t x, gfx_pos_t y, uint32_t u, uint16_t size, uint16_t weight, uint8_t aa, uint8_t italic)
{                               // Plot a character, allow for antialiasing (aa), weight and size are pixel based
   if (u < 32)
      return;
   if (!aa || size == 1)
      aa = 1;
   size *= aa;
   weight *= aa;
   uint8_t *start,
    *end;
   {                            // Find character start/end
      int q;
      if (u < 128)
         q = u - 32;
      else
#if	CONFIG_GFX_UNICODE
      {
         for (q = 0; q < sizeof (font_vector_unicode) / sizeof (*font_vector_unicode); q++)
            if (font_vector_unicode[q] == u)
               break;
         if (q == sizeof (font_vector_unicode) / sizeof (*font_vector_unicode))
            return;
         q += 96;
      }
#else
         return;
#endif
      start = font_vector_data + font_vector_offset[q];
      end = font_vector_data + font_vector_offset[q + 1];
   }
   gfx_pos_t offset = weight;
   if (weight == 3)
      weight--;                 // Adjust else 3 is a square which looks chunky
   const uint8_t max_runs = 6;  // Worst case
   uint8_t runs[aa];
   gfx_pos_t *run[aa];
   for (int i = 0; i < aa; i++)
      run[i] = malloc (sizeof (gfx_pos_t) * 2 * max_runs);
   for (gfx_pos_t Y = 1; Y < size * 9 * 2; Y += 2)
   {                            // Scan lines
      uint8_t sub = Y / 2 % aa;
      runs[sub] = 0;
      gfx_pos_t i = 0;
      if (italic)
         i = size * (size * 9 * 2 - 1 - Y) / (size * 9 * 2);
      for (uint8_t * d = start; d < end; d++)
      {
         uint8_t b1 = *d;
         gfx_pos_t x1 = ((b1 >> 4 & 7) * size * 2 + offset);
         gfx_pos_t y1 = ((b1 & 15) * size * 2 + offset);
         if (Y >= y1 - weight && Y <= y1 + weight)
         {                      // Possible dot
            gfx_pos_t w = weight;
            if (Y != y1)
            {
               uint8_t d;
               if (Y < y1)
                  d = (y1 - Y) * 255 / weight;
               else
                  d = (Y - y1) * 255 / weight;
               w = circle256[d] * weight / 255;
            }
            runs[sub] = add_run (run[sub], runs[sub], max_runs, i + (x1 - w) / 2, i + (x1 + w + 1) / 2);
         }
         if (d + 1 >= end)
            continue;
         uint8_t b2 = d[1];
         if (b2 & 0x80)
            continue;
         gfx_pos_t x2 = ((b2 >> 4 & 7) * size * 2 + offset);
         gfx_pos_t y2 = ((b2 & 15) * size * 2 + offset);
         if (y2 < y1)
         {
            gfx_pos_t s;
            s = x1;
            x1 = x2;
            x2 = s;
            s = y1;
            y1 = y2;
            y2 = s;
         }
         if (Y >= y1 - weight && Y <= y2 + weight)
         {                      // Possible line
            if (y1 == y2)
            {                   // Horizontal, simple
               if (x1 < x2)
                  runs[sub] = add_run (run[sub], runs[sub], max_runs, i + x1 / 2, i + (x2 + 1) / 2);
               else
                  runs[sub] = add_run (run[sub], runs[sub], max_runs, i + x2 / 2, i + (x1 + 1) / 2);
            } else if (x1 == x2)
            {                   // Vertical, simple
               if (Y >= y1 && Y <= y2)
                  runs[sub] = add_run (run[sub], runs[sub], max_runs, i + (x1 - weight) / 2, i + (x1 + weight + 1) / 2);
            } else
            {                   // Diagonal (we cheat knowing 45 degress)
               gfx_pos_t d = (int32_t) weight * 7071 / 10000;
               if (Y >= y1 - d && Y <= y2 + d)
               {
                  gfx_pos_t l = 0,
                     r = 0;
                  if (x1 < x2)
                  {             // right
                     if (Y < y1 + d)
                        l = x1 + (y1 - Y);
                     else
                        l = x1 - d * 2 + (Y - y1);
                     if (Y < y2 - d)
                        r = x2 + d * 2 - (y2 - Y);
                     else
                        r = x2 + (y2 - Y);

                  } else
                  {             // left
                     if (Y < y2 - d)
                        l = x2 - d * 2 + (y2 - Y);
                     else
                        l = x2 - (y2 - Y);
                     if (Y > y1 + d)
                        r = x1 + d * 2 + (y1 - Y);
                     else
                        r = x1 - (y1 - Y);
                  }
                  runs[sub] = add_run (run[sub], runs[sub], max_runs, i + l / 2, i + (r + 1) / 2);
               }
            }
         }
      }
      if (sub == aa - 1)
         plot_runs (p, x, y + Y / 2 / aa, aa, runs, run);
   }
   for (int i = 0; i < aa; i++)
      free (run[i]);
}

void
gfx_7seg_size (uint8_t flags, int8_t size, const char *t, gfx_pos_t * wp, gfx_pos_t * hp)
{
   if (wp)
      *wp = 0;
   if (hp)
      *hp = 0;
   if (size < 1)
      size = 1;
   gfx_pos_t w = 0;
   for (const char *p = t; *p; p++)
      if (strchr (sevensegchar, *p))
      {
         w += 6 * size;
         if (p[1] == ':' || p[1] == '.')
            w += size;
         if ((p[1] == '.' && (flags & GFX_7SEG_SMALL_DOT)) || (p[1] == ':' && (flags & GFX_7SEG_SMALL_COLON)))
            size = ((size / 2) ? : 1);
      }
   if (flags & GFX_7SEG_ITALIC)
      w += size;
   if (wp)
      *wp = w;
   if (hp)
      *hp = 9 * size;
}

void
gfx_7seg (uint8_t flags, int8_t size, const char *fmt, ...)
{                               // Plot 7 segment digits
   if (!gfx)
      return;
   if (size < 1)
      size = 1;
   va_list ap;
   char *temp;
   va_start (ap, fmt);
   vasprintf (&temp, fmt, ap);
   va_end (ap);
   if (!temp)
      return;

   int fontw = 7 * size;        // pixel width of characters in font file

   gfx_pos_t x,
     y,
     w,
     h;
   gfx_7seg_size (flags, size, temp, &w, &h);
   gfx_draw (w, h, size, size, &x, &y); // starting point
   x += size * 9 / 20;          // Better alignment in box
   if (y < gfx_height () && y + size * 9 >= 0)
   {
#if	GFX_BPP <= 2
      const uint8_t aa = 1;
#else
      const uint8_t aa = 4;
#endif
      const uint8_t max_runs = 4;
      gfx_pos_t *a[aa];
      uint8_t an[aa];
      gfx_pos_t *c[aa];
      uint8_t cn[aa];
      for (int r = 0; r < aa; r++)
      {
         a[r] = alloca (sizeof (gfx_pos_t) * max_runs * 2);
         c[r] = alloca (sizeof (gfx_pos_t) * max_runs * 2);
      }
      for (char *p = temp; *p; p++)
      {
         char *m = strchr (sevensegchar, *p);
         if (!m)
            continue;
         uint8_t segs = 7;
         uint16_t map = 0;
         map = sevensegmap[m - sevensegchar];
         if (p[1] == ':' || p[1] == '.')
         {
            segs = 10;
            if (p[1] == ':')
               map |= 0x180;
            else
               map |= 0x200;
         }
         if (x < gfx_width () && x + (segs > 7 ? fontw : 6 * size) >= 0)
         {                      // Plot digit
            const uint16_t unit = width_7seg / 7 / aa;
            const uint16_t base = unit / size / 2;
            uint8_t *i = pack_7seg,
               *e = pack_7seg + sizeof (pack_7seg);
            uint32_t y7 = 0;
            while (i < e)
            {
               uint8_t dup = (*i & 0xF) + 1;
               while (dup--)
               {
                  int yy = y7 * size / unit;
                  uint8_t sub = yy % aa;
                  if (y7 == base + yy * unit / size)
                  {
                     if (!sub)
                     {
                        memset (an, 0, aa);
                        memset (cn, 0, aa);
                     }
                     uint8_t *I = i;
                     uint8_t seg = (*I++ >> 4);
                     uint32_t x7 = 0;
                     if (flags & GFX_7SEG_ITALIC)
                        x7 = width_7seg * (height_7seg - 1 - y7) / height_7seg / 7;
                     while (seg--)
                     {
                        uint8_t S = (*I >> 4);
                        x7 += ((*I & 0xF) << 6) + (I[1] >> 2);
                        gfx_pos_t l = x7 * size / unit;
                        x7 += ((I[1] & 3) << 8) + I[2];
                        gfx_pos_t r = x7 * size / unit;
                        an[sub] = add_run (a[sub], an[sub], max_runs, l, r);
                        if (map & (1 << S))
                           cn[sub] = add_run (c[sub], cn[sub], max_runs, l, r);
                        I += 3;
                     }
                     if (sub == aa - 1)
                     {
                        plot_runs (gfx_pixel0, x, y + yy / aa, aa, an, a);      // background clear
                        plot_runs (gfx_pixel, x, y + yy / aa, aa, cn, c);       // plot
                     }
                  }
                  y7++;
               }
               i += 1 + (*i >> 4) * 3;
            }
         }
         x += (segs > 7 ? fontw : 6 * size);
         if ((p[1] == '.' && (flags & GFX_7SEG_SMALL_DOT)) || (p[1] == ':' && (flags & GFX_7SEG_SMALL_COLON)))
         {
            y += size * 9;
            size = ((size / 2) ? : 1);
            y -= size * 9;
            fontw = 7 * size;   // pixel width of characters in font file
         }
      }
   }
   free (temp);
}

static uint32_t
cwidth (uint8_t flags, uint8_t size, int c)
{                               // character width as printed - some characters are done narrow, and <' ' is fixed size move
   if (size)
   {
      if (c <= 8)
         return c * size;       // Chars 1-8 are spacers
      if (c < ' ')
         return 0;
      if (!(flags & GFX_TEXT_FIXED) && (c == ':' || c == '.' || c == '!' || c == '|' || c == 161))
         return size * 2;
   }
   return 6 * size;
}

static int
utf8 (const char **pp)
{
   if (!pp)
      return -1;
   const char *p = *pp;
   if (!p)
      return -1;
   if (!*p)
      return 0;
   int i = 0;
   if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80)
   {
      i = ((p[0] & 0x1F) << 6) + (p[1] & 0x3F);
      p += 2;
   } else if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80)
   {
      i = ((p[0] & 0xF) << 12) + ((p[1] & 0x3F) << 6) + (p[2] & 0x3F);
      p += 3;
   } else if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80)
   {
      i = ((p[0] & 0xF) << 18) + ((p[1] & 0x3F) << 12) + ((p[2] & 0x3F) << 6) + (p[3] & 0x3F);
      p += 4;
   } else
      i = *p++;
   *pp = p;
   return i;
}

static void
gfx_text_draw_size (uint8_t flags, uint8_t size, const char *text, gfx_pos_t * wp, gfx_pos_t * hp)
{
   if (wp)
      *wp = 0;
   if (hp)
      *hp = 0;
   if (!size)
      return;
   gfx_pos_t x = 0,
      y = 0,
      w = 0,
      h = 0;
   uint8_t z = 7;
   if (flags & GFX_TEXT_DESCENDERS)
      z = 9;
   const char *p = text;
   int c;
   while ((c = utf8 (&p)) > 0)
   {
      if (c == '\n')
      {
         if (x > w)
            w = x;
         x = 0;
         if (*p)
            y++;
         continue;
      }
      x += cwidth (flags, size, c);
   }
   if (x > w)
      w = x;
   if (w && !(flags & GFX_TEXT_ITALIC))
      w -= size;                // Margin right hand pixel needs removing from width
   if (!w)
      return;                   // nothing to print
   h = ((y + 1) * (z + 1) - 1) * size;  // Margin bottom needs removing
   if (wp)
      *wp = w;
   if (hp)
      *hp = h;
}

void
gfx_vector_draw (uint8_t flags, int8_t size, const char *text)
{
   if (!gfx || !size)
      return;
   uint8_t z = 7;
   if (flags & GFX_TEXT_DESCENDERS)
      z = 9;

   gfx_pos_t x,
     y,
     w,
     h;
   gfx_text_draw_size (flags, size, text, &w, &h);

   int fonth = (z + 1) * size;
   gfx_pos_t ox = 0,
      oy = 0;
   gfx_draw (w, h, 1, 1, &ox, &oy);     // starting point
   if (f != b)
      for (x = -1; x <= w; x++)
         for (y = -1; y <= h; y++)
            gfx_pixel (ox + x, oy + y, 0);      // background
   x = y = 0;
   int s1 = size;               // Stroke size
   if ((flags & GFX_TEXT_LIGHT) && size > 1)
      s1 = s1 * 2 / 3;
   const char *p = text;
   int c;
   while ((c = utf8 (&p)) > 0)
   {
      if (!x && y + fonth > h)
         fonth = h - y;         // Last line
      int charw = cwidth (flags, size, c);
      if (charw)
      {
         if (c <= 8)
            c = ' ';
         if (!cwidth (flags, size, *p))
            charw -= size;      // Crop right edge border - messy for UTF8 but should be OK
         if (ox + x + gfx_width () >= 0 && ox + x < gfx_width () && oy + y + size * 9 >= 0 && oy + y < gfx_height ())
         {                      // On screen
            int dx = size * ((cwidth (flags, 1, c) == 2) ? 2 : 0);      // Narrow are offset
#if	GFX_BPP <= 2
            plot_5x9 (gfx_pixel, ox + x - dx, oy + y, c, size, s1, 1, flags & GFX_TEXT_ITALIC);
#else
            plot_5x9 (gfx_pixel, ox + x - dx, oy + y, c, size, s1, 4, flags & GFX_TEXT_ITALIC);
#endif
         }
         x += charw;
      }
      if (!*p || *p == '\n')
      {                         // End of line
         x = 0;
         y += fonth;
         continue;
      }
   }
}

uint8_t
gfx_text_desc (const char *c)
{
   uint8_t n = 0;
   if (c)
      while (*c && n < 255)
         if (strchr ("gijqpy_", *c++))
            n++;
   return n;
}

void
gfx_text (uint8_t flags, uint8_t size, const char *fmt, ...)
{                               // Size negative for descenders
   if (!gfx)
      return;
   va_list ap;
   char *temp;
   va_start (ap, fmt);
   vasprintf (&temp, fmt, ap);
   va_end (ap);
   if (temp)
      gfx_vector_draw (flags, size, temp);
   free (temp);
}

void
gfx_text_size (uint8_t flags, uint8_t size, const char *t, gfx_pos_t * w, gfx_pos_t * h)
{
   gfx_text_draw_size (flags, size, t, w, h);
}

static void
gfx_update (void)
{                               // Update
   if (!gfx)
      return;
   const char *e = gfx_driver_send ();
   if (e)
      ESP_LOGE (TAG, "Fail: %s", e);
   else
      gfx_settings.changed = 0;
   gfx_settings.norefresh = 1;
}

static void
gfx_task (void *p)
{
   while (1)
   {
      if (!gfx_settings.changed)
      {
         usleep (10000);
         continue;
      }
      gfx_lock ();
      gfx_update ();
      gfx_unlock ();
   }
}

const char *
gfx_init_opts (gfx_init_t o)
{                               // Start OLED task and display
   if (gfx)
      return "Already running";
   // Defaults
   if (!o.contrast)
      o.contrast = 255;
#ifdef	CONFIG_IDF_TARGET_ESP32S3
   if (!o.port)
      o.port = SPI3_HOST;
#else
   if (!o.port)
      o.port = HSPI_HOST;
#endif
   if (!o.sck)
      o.sck = CONFIG_GFX_SCK;
   if (!o.rst)
      o.rst = CONFIG_GFX_RST;
   if (!o.dc)
      o.dc = CONFIG_GFX_DC;
   if (!o.cs)
      o.cs = CONFIG_GFX_CS;
   if (!o.miso)
      o.miso = CONFIG_GFX_MISO;
   if (!o.mosi)
      o.mosi = CONFIG_GFX_MOSI;
   if (!o.ena)
      o.ena = CONFIG_GFX_ENA;
   if (!o.busy)
      o.busy = CONFIG_GFX_BUSY;
   if (!o.flip)
      o.flip = CONFIG_GFX_FLIP;
   if (!o.width)
      o.width = GFX_DEFAULT_WIDTH;
   if (!o.height)
      o.height = GFX_DEFAULT_HEIGHT;
   // Check
   if (!o.mosi)
      return "MOSI not set";
   if (!o.sck)
      return "SCK not set";
   if (!o.dc)
      return "DC not set";
   if (!GPIO_IS_VALID_OUTPUT_GPIO (o.mosi))
      return "MOSI not output";
   if (!GPIO_IS_VALID_OUTPUT_GPIO (o.sck))
      return "SCK not output";
   if (!GPIO_IS_VALID_OUTPUT_GPIO (o.dc))
      return "DC not output";
   if (o.ena && !GPIO_IS_VALID_OUTPUT_GPIO (o.ena))
      return "ENA not output";
   if (o.rst && !GPIO_IS_VALID_OUTPUT_GPIO (o.rst))
      return "RST not output";
   if (o.pwr && !GPIO_IS_VALID_OUTPUT_GPIO (o.pwr))
      return "PWR not output";
   if (o.bl && !GPIO_IS_VALID_OUTPUT_GPIO (o.bl))
      return "BL not output";
   gfx_settings = o;
   gfx = malloc (GFX_SIZE);
   if (!gfx)
      return "Malloc fail!";
   if (o.direct)
      gfx_mutex = xSemaphoreCreateCounting (10, 10);    // Shared text access
   else
   {
      gfx_mutex = xSemaphoreCreateMutex ();
      xSemaphoreGive (gfx_mutex);
   }
   memset (gfx, 0, GFX_SIZE);
   spi_bus_config_t config = {
      .mosi_io_num = gfx_settings.mosi,
      .miso_io_num = -1,
      .sclk_io_num = gfx_settings.sck,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = GFX_PAGE,
      .flags = SPICOMMON_BUSFLAG_MASTER,
   };
   if (config.max_transfer_sz > SPI_MAX)
      config.max_transfer_sz = SPI_MAX;
#ifndef  CONFIG_IDF_TARGET_ESP32S3
   if (gfx_settings.port == HSPI_HOST && gfx_settings.mosi == 22 && gfx_settings.sck == 18 && gfx_settings.cs == 5)
      config.flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
   if (spi_bus_initialize (gfx_settings.port, &config, 2))
      return "Init?";
#else
   if (spi_bus_initialize (gfx_settings.port, &config, SPI_DMA_CH_AUTO))
      return "Init?";
#endif
   spi_device_interface_config_t devcfg = {
      .clock_speed_hz = SPI_MASTER_FREQ_20M,
      .mode = 0,
      .spics_io_num = gfx_settings.cs ? : -1,
      .queue_size = 1,
      //.flags = SPI_DEVICE_3WIRE,
   };
   if (spi_bus_add_device (gfx_settings.port, &devcfg, &gfx_spi))
      return "Add?";
   if (gfx_settings.pwr)
   {
      gpio_reset_pin (gfx_settings.pwr);
      gpio_set_direction (gfx_settings.pwr, GPIO_MODE_OUTPUT);
#ifdef	GFX_PWR_LOW
      gpio_set_level (gfx_settings.pwr, 0);     // Power
#else
      gpio_set_level (gfx_settings.pwr, 1);     // Power
#endif
      usleep (100000);
   }
   if (gfx_settings.bl)
   {
      gpio_reset_pin (gfx_settings.bl);
      gpio_set_direction (gfx_settings.bl, GPIO_MODE_OUTPUT);
   }
   if (gfx_settings.ena)
   {
      gpio_reset_pin (gfx_settings.ena);
      gpio_set_direction (gfx_settings.ena, GPIO_MODE_OUTPUT);
#ifdef	GFX_ENA_LOW
      gpio_set_level (gfx_settings.ena, 0);     // Enable
#else
      gpio_set_level (gfx_settings.ena, 1);     // Enable
#endif
   }
   gpio_reset_pin (gfx_settings.dc);
   gpio_set_direction (gfx_settings.dc, GPIO_MODE_OUTPUT);
   if (gfx_settings.busy)
   {
      gpio_reset_pin (gfx_settings.busy);
      gpio_pullup_dis (gfx_settings.busy);
      gpio_set_direction (gfx_settings.busy, GPIO_MODE_INPUT);
   }
   if (gfx_settings.rst)
   {
      gpio_reset_pin (gfx_settings.rst);
      gpio_set_level (gfx_settings.rst, 1);
      gpio_set_direction (gfx_settings.rst, GPIO_MODE_OUTPUT);
      if (!gfx_settings.sleep)
      {
         usleep (100000);
         gpio_set_level (gfx_settings.rst, 0);
         usleep (100000);
         gpio_set_level (gfx_settings.rst, 1);
         usleep (100000);
      }
   }
   if (gfx_settings.sleep)
      gfx_settings.asleep = 1;
   if (!gfx_settings.sleep)
   {
      const char *e = gfx_driver_init ();
      if (e)
      {
         ESP_LOGE (TAG, "Configuration failed %s", e);
         free (gfx);
         gfx = NULL;
         gfx_settings.port = -1;
      }
      gfx_settings.update = 1;
   }
   if (!gfx_settings.direct && gfx)
      xTaskCreate (gfx_task, "GFX", 4 * 1024, NULL, 2, &gfx_task_id);   // Start update task
   return NULL;
}

void
gfx_lock (void)
{                               // Lock display task
   if (!gfx)
      return;
   xSemaphoreTake (gfx_mutex, portMAX_DELAY);
   // preset state
#if GFX_BPP > 2                 // Assume dark
   gfx_background ('k');
   gfx_colour ('w');
#else // Assume light
   gfx_background ('W');
   gfx_colour ('K');
#endif
   gfx_pos (0, 0, GFX_L | GFX_T | GFX_H);
}

void
gfx_unlock (void)
{                               // Unlock display task
   if (!gfx)
      return;
   xSemaphoreGive (gfx_mutex);
   if (gfx_settings.changed && gfx_settings.direct && uxSemaphoreGetCount (gfx_mutex) == 10)
      gfx_update ();
}

void
gfx_refresh (void)
{                               // For e-paper force full refresh
   if (!gfx)
      return;
   gfx_settings.norefresh = 0;
   gfx_settings.changed = 1;
}

void
gfx_force (void)
{                               // Force refresh
   gfx_lock ();
   gfx_settings.changed = 1;
   gfx_unlock ();
}

void
gfx_wait (void)
{                               // Wait for changes to be applied
   if (!gfx)
      return;
   if (gfx_settings.direct && gfx_settings.changed)
      gfx_update ();
   int try = 100;
   while (gfx_settings.changed && try--)
   {
      usleep (10000);
      if (gfx_settings.direct)
         gfx_update ();
   }
}

void
gfx_message (const char *m)
{
   if (!gfx)
      return;
   gfx_lock ();
   gfx_pos (gfx_width () / 2, 0, GFX_T | GFX_C | GFX_V);
   int8_t size = (gfx_width () > 256 ? 6 : 2);
   uint8_t flags = 0;
   while (*m)
   {
      if (*m == '[')
      {
         int8_t isf = 1;
         int s = 0;
         flags = 0;
         for (; *m && *m != ']'; m++)
            if (*m == '_')
               flags |= GFX_TEXT_DESCENDERS;
            else if (*m == '|')
               flags |= GFX_TEXT_LIGHT;
            else if (isdigit ((unsigned char) *m))
               s = s * 10 + *m - '0';
            else if (isalpha ((unsigned char) *m))
            {                   /* colour */
               if ((isf++) & 1)
                  gfx_colour (*m);
               else
                  gfx_background (*m);
            }
         if (*m)
            m++;
         if (s)
            size = s;
      }
      if (!gfx_y ())
         gfx_clear (0);         // Done after setting initial background
      const char *e = m;
      while (*e && *e != '/' && *e != '[')
         e++;
      gfx_text (flags, size, "%.*s", (int) (e - m), m);
      m = e;
      if (*m == '/')
      {
         gfx_pos (gfx_x (), gfx_y () + size - 1, gfx_a ());
         m++;
      }
   }
   gfx_unlock ();
}

int
gfx_ok (void)
{
   if (gfx)
      return 1;
   else
      return 0;
}

void
gfx_sleep (void)
{
   if (!gfx)
      return;
   gfx_driver_sleep ();
}

void
gfx_flip (uint8_t flip)
{
   if (!gfx)
      return;
   gfx_settings.flip = flip;
   gfx_settings.changed = 1;
}

void
gfx_border (uint8_t border)
{
   if (!gfx)
      return;
   gfx_settings.border = border;
   gfx_settings.changed = 1;
}

void
gfx_line (gfx_pos_t x1, gfx_pos_t y1, gfx_pos_t x2, gfx_pos_t y2, gfx_intensity_t l)
{                               // Draw a line
   if (!gfx)
      return;
   gfx_pos_t dx = (x2 - x1),
      adx = dx,
      sdx = 1;
   gfx_pos_t dy = (y2 - y1),
      ady = dy,
      sdy = 1;
   if (dx < 0)
      adx = (sdx = -1) * dx;
   if (dy < 0)
      ady = (sdy = -1) * dy;
   if (dx || dy)
   {
      if (adx > ady)
      {
         gfx_pos_t d = adx / 2;
         while (x1 != x2)
         {
            gfx_pixel (x1, y1, 255);
            d += ady;
            if (d >= adx)
            {
               d -= adx;
               y1 += sdy;
            }
            x1 += sdx;
         }
      } else
      {
         gfx_pos_t d = ady / 2;
         while (y1 != y2)
         {
            gfx_pixel (x1, y1, 255);
            d += adx;
            if (d >= ady)
            {
               d -= ady;
               x1 += sdx;
            }
            y1 += sdy;
         }
      }
   }
   gfx_pixel (x1, y1, 255);
}

uint16_t
gfx_raw_w (void)
{                               // Raw frame buffer width
   return gfx_settings.width;
}

uint16_t
gfx_raw_h (void)
{                               // Raw frame buffer height
   return gfx_settings.height;
}

void *
gfx_raw_b (void)
{                               // Raw frame buffer
   return gfx;
}
#endif
