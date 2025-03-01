/*
 * Simple display and text logic Copyright ©2019-22 Adrian Kennard, Andrews & Arnold Ltd
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

static __attribute__((unused))
     const char TAG[] = "GFX";

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

#define	SPI_MAX	16384

#ifdef	CONFIG_GFX_BUILD_SUFFIX_GFXNONE
     const char *gfx_init_opts (gfx_init_t o)
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
gfx_text (int8_t size, const char *fmt, ...)
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
#else

// general global stuff
     static gfx_init_t gfx_settings = { };

static TaskHandle_t gfx_task_id = NULL;
static SemaphoreHandle_t gfx_mutex = NULL;
static spi_device_handle_t gfx_spi;

// Driver support
static esp_err_t gfx_send_command (uint8_t cmd);
static void gfx_busy_wait (void);       // Manual wait if no busy set
static esp_err_t gfx_send_gfx (uint8_t);
static esp_err_t gfx_send_data (const void *data, uint32_t len);
static esp_err_t gfx_command (uint8_t c, const uint8_t * buf, uint8_t len);
static __attribute__((unused)) esp_err_t gfx_command1 (uint8_t cmd, uint8_t a);
static __attribute__((unused)) esp_err_t gfx_command2 (uint8_t cmd, uint8_t a, uint8_t b);
static __attribute__((unused)) esp_err_t gfx_command4 (uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
static __attribute__((unused)) esp_err_t gfx_command_bulk (const uint8_t * init_code);

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

#ifdef	CONFIG_GFX_7SEG
#include "pack7seg1.h"
#include "pack7seg2.h"
#include "pack7seg3.h"
#include "pack7seg4.h"
#include "pack7seg5.h"
#include "pack7seg6.h"
#include "pack7seg7.h"
#include "pack7seg8.h"
#include "pack7seg9.h"
#ifdef  CONFIG_GFX_BIGDIGIT
#include "pack7seg10.h"
#include "pack7seg11.h"
#include "pack7seg12.h"
#include "pack7seg13.h"
#include "pack7seg14.h"
#include "pack7seg15.h"
#include "pack7seg16.h"
#include "pack7seg17.h"
#include "pack7seg18.h"
#include "pack7seg19.h"
#include "pack7seg20.h"
#endif
#endif

#if	GFX_BPP <= 2
#ifdef	CONFIG_GFX_FONT0
#include "packmono0.h"
#endif
#ifdef	CONFIG_GFX_FONT1
#include "packmono1.h"
#endif
#ifdef	CONFIG_GFX_FONT2
#include "packmono2.h"
#endif
#ifdef	CONFIG_GFX_FONT3
#include "packmono3.h"
#endif
#ifdef	CONFIG_GFX_FONT4
#include "packmono4.h"
#endif
#ifdef	CONFIG_GFX_FONT5
#include "packmono5.h"
#endif
#ifdef	CONFIG_GFX_FONT6
#include "packmono6.h"
#endif
#ifdef	CONFIG_GFX_FONT7
#include "packmono7.h"
#endif
#ifdef	CONFIG_GFX_FONT8
#include "packmono8.h"
#endif
#ifdef	CONFIG_GFX_FONT9
#include "packmono9.h"
#endif
#else
#ifdef	CONFIG_GFX_FONT0
#include "packgrey0.h"
#endif
#ifdef	CONFIG_GFX_FONT1
#include "packgrey1.h"
#endif
#ifdef	CONFIG_GFX_FONT2
#include "packgrey2.h"
#endif
#ifdef	CONFIG_GFX_FONT3
#include "packgrey3.h"
#endif
#ifdef	CONFIG_GFX_FONT4
#include "packgrey4.h"
#endif
#ifdef	CONFIG_GFX_FONT5
#include "packgrey5.h"
#endif
#ifdef	CONFIG_GFX_FONT6
#include "packgrey6.h"
#endif
#ifdef	CONFIG_GFX_FONT7
#include "packgrey7.h"
#endif
#ifdef	CONFIG_GFX_FONT8
#include "packgrey8.h"
#endif
#ifdef	CONFIG_GFX_FONT9
#include "packgrey9.h"
#endif
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

static uint8_t const *const *sevenseg[] = {
#ifdef	CONFIG_GFX_7SEG
   gfx_7seg_pack1,
   gfx_7seg_pack2,
   gfx_7seg_pack3,
   gfx_7seg_pack4,
   gfx_7seg_pack5,
   gfx_7seg_pack6,
   gfx_7seg_pack7,
   gfx_7seg_pack8,
   gfx_7seg_pack9,
#ifdef	CONFIG_GFX_BIGDIGIT
   gfx_7seg_pack10,
   gfx_7seg_pack11,
   gfx_7seg_pack12,
   gfx_7seg_pack13,
   gfx_7seg_pack14,
   gfx_7seg_pack15,
   gfx_7seg_pack16,
   gfx_7seg_pack17,
   gfx_7seg_pack18,
   gfx_7seg_pack19,
   gfx_7seg_pack20,
#endif
#endif
};

static uint8_t const *const *fonts[] = {
#ifdef	CONFIG_GFX_FONT0
   gfx_font_pack0,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT1
   gfx_font_pack1,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT2
   gfx_font_pack2,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT3
   gfx_font_pack3,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT4
   gfx_font_pack4,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT5
   gfx_font_pack5,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT6
   gfx_font_pack6,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT7
   gfx_font_pack7,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT8
   gfx_font_pack8,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT9
   gfx_font_pack9,
#else
   NULL,
#endif
};

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

#define	BLACK	0
#define WHITE   1

#elif GFX_BPP == 2              // Black/red/white

#define	BLACK	0
#define WHITE   1
#define	RED	2

#elif GFX_BPP <= 8              // Greyscale or mono

#define	BLACK	0
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
}

static esp_err_t
gfx_send_command (uint8_t cmd)
{
   ESP_LOGD (TAG, "Command %02X", cmd);
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
   esp_err_t e = spi_device_polling_transmit (gfx_spi, &t);
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
      ESP_LOGD (TAG, "Send %lu", l);
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
   ESP_LOGD (TAG, "Sent");
   return 0;
}

static esp_err_t
gfx_send_gfx (uint8_t page)
{
   return gfx_send_data (gfx + page * GFX_PAGE, GFX_PAGE);
}

static esp_err_t
gfx_command (uint8_t c, const uint8_t * buf, uint8_t len)
{
   esp_err_t e = gfx_send_command (c);
   if (!e && len)
      e = gfx_send_data (buf, len);
   return e;
}

static __attribute__((unused))
     esp_err_t gfx_command1 (uint8_t cmd, uint8_t a)
{                               // Send a command with an arg
   esp_err_t e = gfx_send_command (cmd);
   if (e)
      return e;
   gpio_set_level (gfx_settings.dc, 1);
   spi_transaction_t t = {
      .length = 8,
      .tx_data = {a},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_polling_transmit (gfx_spi, &t);
}

static __attribute__((unused))
     esp_err_t gfx_command2 (uint8_t cmd, uint8_t a, uint8_t b)
{                               // Send a command with args
   esp_err_t e = gfx_send_command (cmd);
   if (e)
      return e;
   gpio_set_level (gfx_settings.dc, 1);
   spi_transaction_t t = {
      .length = 16,
      .tx_data = {a, b},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_polling_transmit (gfx_spi, &t);
}

static __attribute__((unused))
     esp_err_t gfx_command4 (uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{                               // Send a command with args
   esp_err_t e = gfx_send_command (cmd);
   if (e)
      return e;
   gpio_set_level (gfx_settings.dc, 1);
   spi_transaction_t t = {
      .length = 32,
      .tx_data = {a, b, c, d},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_polling_transmit (gfx_spi, &t);
}

static __attribute__((unused))
     esp_err_t gfx_command_bulk (const uint8_t * bulk)
{                               // Bulk command
   // bulk is a sequence of blocks of the form :-
   // Len, 0x00 is end, 0xFF is busy wait with no command or data
   // Command (included in len)
   // Data (len-1 bytes)
   uint8_t buf[64];

   while (*bulk)
   {
      uint8_t len = *bulk++;
      if (len > sizeof (buf))
      {
         ESP_LOGE (TAG, "Bad bulk command len %d", len);
         break;
      }
      memcpy (buf, bulk, len);
      bulk += len;
      esp_err_t e = gfx_command (*buf, buf + 1, len - 1);
      if (e)
         return e;
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
   if (i < 255 && f == b)
      return;                   // Mask mode
#if GFX_BPP == 1                // Black/white
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

static __attribute__((unused))
     void gfx_block2N_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, uint8_t mx, uint8_t my,
                            const uint8_t * data)
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

static __attribute__((unused))
     void gfx_mask_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t dx, const uint8_t * data, gfx_intensity_t i)
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

static __attribute__((unused))
     void gfx_block2 (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data, int l)
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

static __attribute__((unused))
     void gfx_block2_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data)
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

static __attribute__((unused))
     void gfx_block16 (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, const uint8_t * data)
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

static __attribute__((unused))
     void gfx_block16_pack (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data)
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

void
gfx_7seg_size (int8_t size, const char *t, gfx_pos_t * wp, gfx_pos_t * hp)
{
   if (wp)
      *wp = 0;
   if (hp)
      *hp = 0;
   if (size < 1)
      size = 1;
   if (size > sizeof (sevenseg) / sizeof (*sevenseg))
      size = sizeof (sevenseg) / sizeof (*sevenseg);
   gfx_pos_t w = 0;
   for (char *p = t; *p; p++)
      if (strchr (sevensegchar, *p))
      {
         w += 6 * size;
         if (p[1] == ':' || p[1] == '.')
            w += size;
      }
   if (wp)
      *wp = w;
   if (hp)
      *hp = 9 * size;
}

void
gfx_7seg (int8_t size, const char *fmt, ...)
{                               // Plot 7 segment digits
   if (!gfx)
      return;
   if (size < 1)
      size = 1;
   if (size > sizeof (sevenseg) / sizeof (*sevenseg))
      size = sizeof (sevenseg) / sizeof (*sevenseg);
   if (!gfx || size < 1 || !sevenseg[size - 1])
      return;
   va_list ap;
   char temp[gfx_width () / 4 + 2];
   va_start (ap, fmt);
   vsnprintf (temp, sizeof (temp), fmt, ap);
   va_end (ap);

   int fontw = 7 * size;        // pixel width of characters in font file
   inline const uint8_t *fontdata (uint8_t s)
   {
      return sevenseg[size - 1][s];
   }

   gfx_pos_t x,
     y,
     w,
     h;
   gfx_7seg_size (size, temp, &w, &h);
   gfx_draw (w, h, size, size, &x, &y); // starting point
   x += size / 2;               // Better alignment in box
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
      for (int s = 0; s < segs; s++)
         gfx_mask_pack (x, y, 0, fontdata (s), (map & (1 << s)) ? 255 : 0);
      x += (segs > 7 ? fontw : 6 * size);
   }
}

static int
cwidth (int8_t size, char c)
{                               // character width as printed - some characters are done narrow, and <' ' is fixed size move
   if (c & 0x80)
      return 0;
   if (size)
   {
      if (c <= 8)
         return c * size;       // Chars 1-8 are spacers
      if (c < ' ')
         return 0;
      if (c == ':' || c == '.')
         return size * 2;
   }
   return (size ? 6 * size : 4);
}

static void
gfx_text_draw_size (int8_t size, uint8_t z, const char *text, gfx_pos_t * wp, gfx_pos_t * hp)
{
   if (wp)
      *wp = 0;
   if (hp)
      *hp = 0;
   gfx_pos_t x = 0,
      y = 0;
   for (const char *p = text; *p; p++)
   {
      if (*p == '\n')
      {
         if (x > w)
            w = x;
         x = 0;
         if (p[1])
            y++;
         continue;
      }
      x += cwidth (size, *p);
   }
   if (x > w)
      w = x;
   if (w)
      w -= (size ? : 1);        // Margin right hand pixel needs removing from width
   if (!w)
      return;                   // nothing to print
   h = (y + 1) * fonth - (size ? : 1);  // Margin bottom needs removing
   if (wp)
      *wp = w;
   if (hp)
      *hp = h;
}

void
gfx_text_draw (int8_t size, uint8_t z, uint8_t blocky, const char *text)
{                               // Size negative for descenders
   if (!gfx || !fonts[size])
      return;

   int fontw = (size ? 6 * size : 4);   // pixel width of characters in font file
   int fonth = (z + 1) * (size ? : 1);

   gfx_pos_t x,
     y,
     w,
     h;
   gfx_text_draw_size (size, x, text);

   const uint8_t *fontdata (char c)
   {
      return fonts[size][c - ' '];
   }

   gfx_pos_t ox = 0,
      oy = 0;
   gfx_draw (w, h, 1, 1, &ox, &oy);     // starting point
   // Border
   for (gfx_pos_t n = -1; n <= w; n++)
   {
      gfx_pixel (ox + n, oy - 1, 0);
      gfx_pixel (ox + n, oy + h, 0);
   }
   for (gfx_pos_t n = 0; n < h; n++)
   {
      gfx_pixel (ox - 1, oy + n, 0);
      gfx_pixel (ox + w, oy + n, 0);
   }
   // Text
   x = y = 0;
   for (const char *p = text; *p; p++)
   {
      if (!x && y + fonth > h)
         fonth = h - y;         // Last line
      int c = *p;
      int charw = cwidth (size, c);
      if (charw)
      {
         if (c <= 8)
            c = ' ';
         if (!cwidth (size, p[1]))
            charw -= (size ? : 1);      // Crop right edge border
         int dx = size * ((c == ':' || c == '.') ? 2 : 0);      // : and . are offset as make narrower
         if (blocky)
         {
#if	GFX_BPP <= 2            // TODO should really do full colour
            gfx_block2N_pack (ox + x, oy + y, charw / size, fonth / (size ? : 1), dx / size, size, size, fonts[1][c - ' ']);
#endif
         } else
         {
#if    GFX_BPP <= 2
            gfx_block2_pack (ox + x, oy + y, charw, fonth, dx, fontdata (c));
#else
            gfx_block16_pack (ox + x, oy + y, charw, fonth, dx, fontdata (c));
#endif
         }
         x += charw;
      }
      if (!p[1] || p[1] == '\n')
      {                         // End of line
         for (gfx_pos_t X = x; X < w; X++)
            for (gfx_pos_t Y = 0; Y < fonth; Y++)
               gfx_pixel (ox + X, oy + y + Y, 0);       // Pack background
         x = 0;
         y += fonth;
         continue;
      }
   }
}

void
gfx_text (int8_t size, const char *fmt, ...)
{                               // Size negative for descenders
   if (!gfx)
      return;
   int z = 7;                   // effective height
   if (size < 0)
   {                            // indicates descenders allowed
      size = -size;
      z = 9;
   } else if (!size)
      z = 5;
   if (size > sizeof (fonts) / sizeof (*fonts) - 1)
      size = sizeof (fonts) / sizeof (*fonts) - 1;
   va_list ap;
   char temp[gfx_width () / 4 + 2];
   va_start (ap, fmt);
   vsnprintf (temp, sizeof (temp), fmt, ap);
   va_end (ap);
   gfx_text_draw (size, z, 0, temp);
}

void
gfx_text_size (int8_t size, const char *t, gfx_pos_t & w, gfx_pos_t & h)
{
   if (w)
      *w = 0;
   if (h)
      *h = 0;
   if (size < 1)
      size = 1;
   if (size > sizeof (sevenseg) / sizeof (*sevenseg))
      size = sizeof (sevenseg) / sizeof (*sevenseg);
}


void
gfx_7seg (int8_t size, const char *fmt, ...)
{                               // Plot 7 segment digits
   if (!gfx)
      return;
   if (size < 1)
      size = 1;
   if (size > sizeof (sevenseg) / sizeof (*sevenseg))
      size = sizeof (sevenseg) / sizeof (*sevenseg);
   if (!gfx || size < 1 || !sevenseg[size - 1])
      return;
   va_list ap;
   char temp[gfx_width () / 4 + 2];
   int z = 7;                   // effective height
   if (size < 0)
   {                            // indicates descenders allowed
      size = -size;
      z = 9;
   } else if (!size)
      z = 5;
   if (size > sizeof (fonts) / sizeof (*fonts) - 1)
      size = sizeof (fonts) / sizeof (*fonts) - 1;
   gfx_texty_draw_size (size, z, t, w, h);
}

void
gfx_blocky (int8_t size, const char *fmt, ...)
{                               // Size negative for descenders, blocky text
   if (!gfx)
      return;
   int z = 7;                   // effective height
   if (size < 0)
   {                            // indicates descenders allowed
      size = -size;
      z = 9;
   } else if (!size)
      z = 5;
   va_list ap;
   char temp[gfx_width () / 4 + 2];
   va_start (ap, fmt);
   vsnprintf (temp, sizeof (temp), fmt, ap);
   va_end (ap);
   gfx_text_draw (size, z, 1, temp);
}

void
gfx_blocky_size (int8_t size, const char *t, gfx_pos_t & w, gfx_pos_t & h)
{
   if (w)
      *w = 0;
   if (h)
      *h = 0;
   if (size < 1)
      size = 1;
   if (size > sizeof (sevenseg) / sizeof (*sevenseg))
      size = sizeof (sevenseg) / sizeof (*sevenseg);
}


void
gfx_7seg (int8_t size, const char *fmt, ...)
{                               // Plot 7 segment digits
   if (!gfx)
      return;
   if (size < 1)
      size = 1;
   if (size > sizeof (sevenseg) / sizeof (*sevenseg))
      size = sizeof (sevenseg) / sizeof (*sevenseg);
   if (!gfx || size < 1 || !sevenseg[size - 1])
      return;
   va_list ap;
   char temp[gfx_width () / 4 + 2];
   int z = 7;                   // effective height
   if (size < 0)
   {                            // indicates descenders allowed
      size = -size;
      z = 9;
   } else if (!size)
      z = 5;
   gfx_texty_draw_size (size, z, t, w, h);
}

static void
gfx_update (void)
{                               // Update
   if (!gfx)
      return;
   if (!gfx_driver_send ())
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
   if (gfx_settings.ena)
   {
      gpio_reset_pin (gfx_settings.ena);
      gpio_set_direction (gfx_settings.ena, GPIO_MODE_OUTPUT);
      gpio_set_level (gfx_settings.ena, 1);     // Enable
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
         usleep (10000);
         gpio_set_level (gfx_settings.rst, 0);
         usleep (10000);
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
   while (*m)
   {
      if (*m == '[')
      {
         int8_t isf = 1;
         int8_t lc = 1;
         for (; *m && *m != ']'; m++)
            if (*m == '-')
               lc = -lc;
            else if (isdigit ((unsigned char) *m))
               size = lc * (*m - '0');  /* size */
            else if (isalpha ((unsigned char) *m))
            {                   /* colour */
               if ((isf++) & 1)
                  gfx_colour (*m);
               else
                  gfx_background (*m);
            }
         if (*m)
            m++;
      }
      if (!gfx_y ())
         gfx_clear (0);         // Done after setting initial background
      const char *e = m;
      while (*e && *e != '/' && *e != '[')
         e++;
      gfx_text (size, "%.*s", (int) (e - m), m);
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
gfx_caffeine (void)
{
   gfx_settings.caffeine = 1;
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
#endif
