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

#define	SPI_MAX	32768

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
static void gfx_busy_wait (const char *);
static esp_err_t gfx_send_command (uint8_t cmd);
static esp_err_t gfx_send_gfx (void);
static esp_err_t gfx_send_data (const void *data, uint32_t len);
static esp_err_t gfx_command (uint8_t c, const uint8_t * buf, uint8_t len);
static __attribute__((unused))
     esp_err_t
     gfx_command1 (uint8_t cmd, uint8_t a);
     static __attribute__((unused))
     esp_err_t
     gfx_command2 (uint8_t cmd, uint8_t a, uint8_t b);
     static __attribute__((unused))
     esp_err_t
     gfx_command4 (uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
     static __attribute__((unused))
     esp_err_t
     gfx_command_list (const uint8_t * init_code);

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
#ifdef  CONFIG_GFX_BUILD_SUFFIX_GD7965
#include "gd7965.c"
#endif

#ifdef	CONFIG_GFX_7SEG
#include "7seg1.h"
#include "7seg2.h"
#include "7seg3.h"
#include "7seg4.h"
#include "7seg5.h"
#include "7seg6.h"
#include "7seg7.h"
#include "7seg8.h"
#endif

#if	GFX_BPP == 1
#ifdef	CONFIG_GFX_FONT0
#include "mono0.h"
#endif
#ifdef	CONFIG_GFX_FONT1
#include "mono1.h"
#endif
#ifdef	CONFIG_GFX_FONT2
#include "mono2.h"
#endif
#ifdef	CONFIG_GFX_FONT3
#include "mono3.h"
#endif
#ifdef	CONFIG_GFX_FONT4
#include "mono4.h"
#endif
#ifdef	CONFIG_GFX_FONT5
#include "mono5.h"
#endif
#ifdef	CONFIG_GFX_FONT6
#include "mono6.h"
#endif
#else
#ifdef	CONFIG_GFX_FONT0
#include "grey0.h"
#endif
#ifdef	CONFIG_GFX_FONT1
#include "grey1.h"
#endif
#ifdef	CONFIG_GFX_FONT2
#include "grey2.h"
#endif
#ifdef	CONFIG_GFX_FONT3
#include "grey3.h"
#endif
#ifdef	CONFIG_GFX_FONT4
#include "grey4.h"
#endif
#ifdef	CONFIG_GFX_FONT5
#include "grey5.h"
#endif
#ifdef	CONFIG_GFX_FONT6
#include "grey6.h"
#endif
#endif

     static uint8_t const
     sevensegmap[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

static uint8_t const *sevenseg[] = {
#ifdef	CONFIG_GFX_7SEG
   gfx_7seg1,
   gfx_7seg2,
   gfx_7seg3,
   gfx_7seg4,
   gfx_7seg5,
   gfx_7seg6,
   gfx_7seg7,
   gfx_7seg8,
#endif
};

static uint8_t const *fonts[] = {
#ifdef	CONFIG_GFX_FONT0
   gfx_font0,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT1
   gfx_font1,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT2
   gfx_font2,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT3
   gfx_font3,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT4
   gfx_font4,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT5
   gfx_font5,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT6
   gfx_font6,
#else
   NULL,
#endif
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

#define WHITE   (RED+GREEN+BLUE)

#elif GFX_BPP <= 8              // Greyscale or mono
#define WHITE   1
#define GFX_INTENSITY_BPP  GFX_BPP
#endif

#if GFX_BPP > 16
typedef uint32_t gfx_cell_t;
#define GFX_SIZE (gfx_settings.width * gfx_settings.height * sizeof(gfx_cell_t))
#elif GFX_BPP > 8
typedef uint16_t gfx_cell_t;
#define GFX_SIZE (gfx_settings.width * gfx_settings.height * sizeof(gfx_cell_t))
#else
typedef uint8_t gfx_cell_t;
#define GFX_SIZE ((gfx_settings.width * GFX_BPP + 7) / 8 * gfx_settings.height)
#endif
static gfx_cell_t *gfx = NULL;

// drawing state
static gfx_pos_t x = 0,
   y = 0;                       // position
static gfx_align_t a = 0;       // alignment and movement
static char f = 0,              // colour
   b = 0;
#if GFX_BPP > 1
static uint32_t f_mul = 0;
static uint32_t b_mul = 0;      // actual f/b colour multiplier
#endif
static uint8_t bw = 0;          // 1bpp foreground colour


// Driver support

static void
gfx_busy_wait (const char *why)
{
   if (!gfx_settings.busy)
   {                            // No busy, so just wait
      sleep (5);
      return;
   }
#ifdef	GFX_BUSY_LOW
   if (gpio_get_level (gfx_settings.busy))
#else
   if (!gpio_get_level (gfx_settings.busy))
#endif
   {
      ESP_LOGD (TAG, "Not busy (%s)", why);
      return;
   }
   uint64_t a = esp_timer_get_time ();
   int try = 10000;
#ifdef	GFX_BUSY_LOW
   while (try-- && !gpio_get_level (gfx_settings.busy))
#else
   while (try-- && gpio_get_level (gfx_settings.busy))
#endif
      usleep (1000);
#ifdef	GFX_BUSY_LOW
   if (!gpio_get_level (gfx_settings.busy))
#else
   if (gpio_get_level (gfx_settings.busy))
#endif
   {
      ESP_LOGD (TAG, "Busy stuck (%s)", why);
      return;
   }
   uint64_t b = esp_timer_get_time ();
   ESP_LOGD (TAG, "Busy waited (%s) %lldms", why, (b - a + 500) / 1000);
}

static esp_err_t
gfx_send_command (uint8_t cmd)
{
   ESP_LOGD (TAG, "Command %02X", cmd);
   gpio_set_level (gfx_settings.dc, 0);
   spi_transaction_t t = {
      .length = 8,
      .tx_data = {cmd},
      .flags = SPI_TRANS_USE_TXDATA,
   };
   esp_err_t e = spi_device_polling_transmit (gfx_spi, &t);
   return e;
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
         ESP_LOGE (TAG, "Failed send data");
         return e;
      }
      len -= l;
      data += l;
   }
   ESP_LOGD (TAG, "Sent");
   return 0;
}

static esp_err_t
gfx_send_gfx (void)
{
   return gfx_send_data (gfx, GFX_SIZE);
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
     esp_err_t gfx_command_list (const uint8_t * init_code)
{
   uint8_t buf[64];

   while (init_code[0] != 0xFE)
   {
      uint8_t cmd = init_code[0];
      init_code++;
      uint8_t num_args = init_code[0];
      init_code++;
      if (cmd == 0xFF)
      {
         gfx_busy_wait ("command_list");
         if (num_args)
            usleep (num_args * 1000);
         continue;
      }
      if (num_args > sizeof (buf))
      {
         ESP_LOGE (TAG, "Bad command_list len %d", num_args);
         break;
      }

      for (int i = 0; i < num_args; i++)
      {
         buf[i] = init_code[0];
         init_code++;
      }
      esp_err_t e = gfx_command (cmd, buf, num_args);
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

#if	GFX_BPP > 1
static uint32_t
gfx_colour_lookup (char c)
{                               // character to colour mapping, default is white
   switch (c)
   {
   case 'k':
   case 'K':
      return BLACK;
#if GFX_BPP > 8
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
#endif

void
gfx_colour (char newf)
{                               // Set foreground
#if GFX_BPP > 1
   f_mul = gfx_colour_lookup (f = newf);
#endif
   bw = (newf == 'K' || newf == 'k' ? 255 : 0);
}

void
gfx_background (char newb)
{                               // Set background
#if GFX_BPP >1
   b_mul = gfx_colour_lookup (b = newb);
#endif
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
   if (!gfx || x < 0 || x >= gfx_settings.width || y < 0 || y >= gfx_settings.height)
      return;                   // out of display
#if GFX_BPP > 1
   if (gfx_settings.contrast < 4)
      i >>= (4 - gfx_settings.contrast + ((x ^ y) & 1));        // Extra dim and dithered
   else if (gfx_settings.contrast < 4)
      i >>= (8 - gfx_settings.contrast);        // Extra dim
#endif
#if GFX_BPP <= 8
   const int bits = (1 << GFX_BPP) - 1;
   const int shift = 8 - (x % (8 / GFX_BPP)) - GFX_BPP;
   const int line = (gfx_settings.width * GFX_BPP + 7) / 8;
   const int addr = line * y + x * GFX_BPP / 8;
   i >>= (8 - GFX_BPP);
   i &= bits;
#ifndef	GFX_INVERT
   i ^= bits;
#endif
   if (((gfx[addr] >> shift) & bits) == i)
      return;
   gfx[addr] = ((gfx[addr] & ~(bits << shift)) | (i << shift));
   gfx_settings.changed = 1;
#else
   uint16_t v = ntohs (f_mul * (i >> (8 - GFX_INTENSITY_BPP)) + b_mul * ((0xFF ^ i) >> (8 - GFX_INTENSITY_BPP)));
   if (v == gfx[(y * gfx_settings.width) + x])
      return;
   gfx[(y * gfx_settings.width) + x] = v;
   gfx_settings.changed = 1;
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

static __attribute__((unused))
     void gfx_mask_block (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, uint8_t mx, uint8_t my,
                          const uint8_t * data, int l, int c)
{                               // Draw a block from 2 bit image data, l is data width for each row, c is colour to plot where icon is black/set
   if (!l)
      l = (w + 7) / 8;          // default is pixels width
   for (gfx_pos_t row = 0; row < h; row++)
   {
      for (gfx_pos_t col = 0; col < w; col++)
         if ((data[(col + dx) / 8] >> ((col + dx) & 7)) & 1)
            for (uint8_t qx = 0; qx < mx; qx++)
               for (uint8_t qy = 0; qy < mx; qy++)
                  gfx_pixel (x + col * mx + qx, y + row * my + qy, c);
      data += l;
   }
}

static __attribute__((unused))
     void gfx_mask (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data, int l, int c)
{                               // Draw a block from 2 bit image data, l is data width for each row, c is colour to plot where icon is black/set
   if (!l)
      l = (w + 7) / 8;          // default is pixels width
   for (gfx_pos_t row = 0; row < h; row++)
   {
      for (gfx_pos_t col = 0; col < w; col++)
         if ((data[(col + dx) / 8] >> ((col + dx) & 7)) & 1)
            gfx_pixel (x + col, y + row, c);
      data += l;
   }
}

static __attribute__((unused))
     void gfx_block16 (gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, gfx_pos_t dx, const uint8_t * data, int l)
{                               // Draw a block from 16 bit greyscale data, l is data width for each row
   if (!l)
      l = (w + 1) / 2;          // default is pixels width
   for (gfx_pos_t row = 0; row < h; row++)
   {
      for (gfx_pos_t col = 0; col < w; col++)
      {
         uint8_t v = data[(col + dx) / 2];
         gfx_pixel (x + col, y + row, (v & 0xF0) | (v >> 4));
         col++;
         if (col < w)
            gfx_pixel (x + col, y + row, (v & 0xF) | (v << 4));
      }
      data += l;
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
#if GFX_BPP > 1
   gfx_colour ('w');
#else
   gfx_colour ('K'); // Default colour black
#endif
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
      gfx_mask (x, y, w, h, 0, data, 0, bw);
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
      gfx_block16 (x, y, w, h, 0, data, 0);
   }
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
   int fonth = 9 * size;        // pixel height of characters in font file
   inline const uint8_t *fontdata (uint8_t s)
   {
      return sevenseg[size - 1] + s * ((fontw + 7) / 8) * fonth;
   }

   int w = 0;
   for (char *p = temp; *p; p++)
      if (*p == ' ' || *p == '-' || isdigit ((int) *p))
      {
         w += 6 * size;
         if (p[1] == ':' || p[1] == '.')
            w += size;
      }
   gfx_pos_t x,
     y;
   gfx_draw (w, 9 * size, size, size, &x, &y);  // starting point
   x += size / 2;               // Better alignment in box
   for (char *p = temp; *p; p++)
   {
      if (*p != ' ' && *p != '-' && !isdigit ((int) *p))
         continue;
      uint8_t segs = 7;
      uint16_t map = 0;
      if (*p == '-')
         map = 0x40;
      else if (isdigit ((int) *p))
         map = sevensegmap[*p - '0'];
      if (p[1] == ':' || p[1] == '.')
      {
         segs = 9;
         map |= 0x80;
         if (p[1] == ':')
            map |= 0x100;
      }
      if (map)
         for (int s = 0; s < segs; s++)
            if (map & (1 << s))
               gfx_mask (x, y, fontw, fonth, 0, fontdata (s), (fontw + 7) / 8, bw);
      x += (segs == 9 ? fontw : 6 * size);
   }
}

void
gfx_text_draw (int8_t size, uint8_t z, uint8_t blocky, const char *text)
{                               // Size negative for descenders
   if (!gfx || !fonts[size])
      return;

   int fontw = (size ? 6 * size : 4);   // pixel width of characters in font file
   int fonth = (size ? 9 * size : 5);   // pixel height of characters in font file

   int w = 0;                   // width of overall text
   int h = z * (size ? : 1);    // height of overall text
   int cwidth (char c)
   {                            // character width as printed - some characters are done narrow, and <' ' is fixed size move
      if (c & 0x80)
         return 0;
      if (size)
      {
         if (c < ' ')
            return c * size;
         if (c == ':' || c == '.')
            return size * 2;
      }
      return fontw;
   }
   const uint8_t *fontdata (char c)
   {
#if	GFX_BPP == 1
      const uint8_t *d = fonts[size] + (c - ' ') * ((fontw + 7) / 8) * fonth;
#else
      const uint8_t *d = fonts[size] + (c - ' ') * fonth * fontw / 2;
#endif
      return d;
   }
   for (const char *p = text; *p; p++)
      w += cwidth (*p);
   gfx_pos_t x,
     y;
   if (w)
      w -= (size ? : 1);        // Margin right hand pixel needs removing from width
   if (!w)
      return;                   // nothing to print
   gfx_draw (w, h, size ? : 1, size ? : 1, &x, &y);     // starting point
#if     GFX_BPP > 1
   // Border
   for (gfx_pos_t n = -1; n <= w; n++)
   {
      gfx_pixel (x + n, y - 1, 0);
      gfx_pixel (x + n, y + h, 0);
   }
   for (gfx_pos_t n = 0; n < h; n++)
   {
      gfx_pixel (x - 1, y + n, 0);
      gfx_pixel (x + w, y + n, 0);
   }
#endif
   for (const char *p = text; *p; p++)
   {
      int c = *p;
      int charw = cwidth (c);
      if (charw)
      {
         if (c < ' ')
            c = ' ';
         if (!p[1])
            charw -= (size ? : 1);
         int dx = size * ((c == ':' || c == '.') ? 2 : 0);      // : and . are offset as make narrower
         if (blocky)
         {
#if	GFX_BPP == 1
            gfx_mask_block (x, y, charw / size, z, dx / size, size, size, fonts[1] + (c - ' ') * 9, 1, bw);
#endif
         } else
         {
#if	GFX_BPP == 1
            gfx_mask (x, y, charw, h, dx, fontdata (c), (fontw + 7) / 8, bw);
#else
            gfx_block16 (x, y, charw, h, dx, fontdata (c), fontw / 2);
#endif
         }
         x += charw;
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
   if (size > sizeof (fonts) / sizeof (*fonts))
      size = sizeof (fonts) / sizeof (*fonts);
   va_list ap;
   char temp[gfx_width () / 4 + 2];
   va_start (ap, fmt);
   vsnprintf (temp, sizeof (temp), fmt, ap);
   va_end (ap);
   gfx_text_draw (size, z, 0, temp);
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
      .max_transfer_sz = GFX_SIZE,
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
         gpio_set_level (gfx_settings.rst, 0);
         usleep (1000);
         gpio_set_level (gfx_settings.rst, 1);
         usleep (1000);
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
#if GFX_BPP > 1
   gfx_background ('k');
   gfx_colour ('w');
#else
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
   uint8_t size = 2;
   while (*m)
   {
      if (*m == '[')
      {
         char isf = 1;
         for (; *m && *m != ']'; m++)
            if (isdigit ((unsigned char) *m))
               size = *m - '0'; /* size */
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
         gfx_clear (0);
      const char *e = m;
      while (*e && *e != '/' && *e != '[')
         e++;
      gfx_text (size, "%.*s", (int) (e - m), m);
      m = e;
      if (*m == '/')
         m++;
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
#endif
