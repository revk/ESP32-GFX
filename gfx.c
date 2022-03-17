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

static const char TAG[] = "OLED";

#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <hal/spi_types.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "gfx.h"

#ifdef	CONFIG_GFX_FONT0
#include "font0.h"
#endif
#ifdef	CONFIG_GFX_FONT1
#include "font1.h"
#endif
#ifdef	CONFIG_GFX_FONT2
#include "font2.h"
#endif
#ifdef	CONFIG_GFX_FONT3
#include "font3.h"
#endif
#ifdef	CONFIG_GFX_FONT4
#include "font4.h"
#endif
#ifdef	CONFIG_GFX_FONT5
#include "font5.h"
#endif

static uint8_t const *fonts[] = {
#ifdef	CONFIG_GFX_FONT0
   font0,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT1
   font1,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT2
   font2,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT3
   font3,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT4
   font4,
#else
   NULL,
#endif
#ifdef	CONFIG_GFX_FONT5
   font5,
#else
   NULL,
#endif
};

#define	BLACK	0
#if CONFIG_GFX_BPP == 16        // 16 bit RGB
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

#elif CONFIG_GFX_BPP <= 8       // Greyscale or mono
#define WHITE   1
#define GFX_INTENSITY_BPP  CONFIG_GFX_BPP
#endif

#if CONFIG_GFX_BPP>16
typedef uint32_t gfx_cell_t;
#define GFX_SIZE (CONFIG_GFX_WIDTH * CONFIG_GFX_HEIGHT * sizeof(gfx_cell_t))
#elif CONFIG_GFX_BPP>8
typedef uint16_t gfx_cell_t;
#define GFX_SIZE (CONFIG_GFX_WIDTH * CONFIG_GFX_HEIGHT * sizeof(gfx_cell_t))
#else
typedef uint8_t gfx_cell_t;
#define GFX_SIZE (CONFIG_GFX_WIDTH * CONFIG_GFX_HEIGHT * CONFIG_GFX_BPP / 8)
#endif
static gfx_cell_t *gfx = NULL;
static gfx_init_t gfx_settings = { };

// general global stuff
static TaskHandle_t gfx_task_id = NULL;
static SemaphoreHandle_t gfx_mutex = NULL;
static int8_t gfx_locks = 0;
static spi_device_handle_t gfx_spi;
static volatile uint8_t gfx_changed = 1;        // Pixels changed
static volatile uint8_t gfx_update = 0; // Other settings changed

// drawing state
static gfx_pos_t x = 0,
    y = 0;                      // position
static gfx_align_t a = 0;       // alignment and movement
static char f = 0,              // colour
    b = 0;
static uint32_t f_mul = 0,
    b_mul = 0;                  // actual f/b colour multiplier


// Driver support

static void gfx_busy_wait(void)
{
   if (!gfx_settings.busy)
   {                            // No busy, so just wait
      sleep(1);
      return;
   }
   int try = 1000;
   while (try--)
   {
      if (gpio_get_level(gfx_settings.busy))
         break;
      usleep(10000);
   }
}

static esp_err_t gfx_send_command(uint8_t cmd)
{
   gpio_set_level(gfx_settings.dc, 0);
   spi_transaction_t t = {
      .length = 8,
      .tx_data = { cmd },
      .flags = SPI_TRANS_USE_TXDATA,
   };
   esp_err_t e = spi_device_polling_transmit(gfx_spi, &t);
   return e;
}

static esp_err_t gfx_send_data(const void *data, uint16_t len)
{
   gpio_set_level(gfx_settings.dc, 1);
   spi_transaction_t c = {
      .length = 8 * len,
      .tx_buffer = data,
   };
   return spi_device_transmit(gfx_spi, &c);
}

static esp_err_t gfx_command(uint8_t c, const uint8_t * buf, uint16_t len)
{
   esp_err_t e = gfx_send_command(c);
   if (!e && len)
      e = gfx_send_data(buf, len);
   return e;
}

static __attribute__((unused)) esp_err_t gfx_command1(uint8_t cmd, uint8_t a)
{                               // Send a command with an arg
   esp_err_t e = gfx_send_command(cmd);
   if (e)
      return e;
   gpio_set_level(gfx_settings.dc, 1);
   spi_transaction_t d = {
      .length = 8,
      .tx_data = { a },
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_polling_transmit(gfx_spi, &d);
}

static __attribute__((unused)) esp_err_t gfx_command2(uint8_t cmd, uint8_t a, uint8_t b)
{                               // Send a command with args
   esp_err_t e = gfx_send_command(cmd);
   if (e)
      return e;
   gpio_set_level(gfx_settings.dc, 1);
   spi_transaction_t d = {
      .length = 16,
      .tx_data = { a, b },
      .flags = SPI_TRANS_USE_TXDATA,
   };
   return spi_device_polling_transmit(gfx_spi, &d);
}

static __attribute__((unused)) esp_err_t gfx_command_list(const uint8_t * init_code)
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
         gfx_busy_wait();
         usleep(num_args * 1000);
         continue;
      }
      if (num_args > sizeof(buf))
      {
         ESP_LOGE(TAG, "Bad command_list len %d", num_args);
         break;
      }

      for (int i = 0; i < num_args; i++)
      {
         buf[i] = init_code[0];
         init_code++;
      }
      esp_err_t e = gfx_command(cmd, buf, num_args);
      if (e)
         return e;
   }
   return 0;
}


// Driver
#ifdef  CONFIG_GFX_SSD1351
#include "ssd1351.c"
#endif
#ifdef  CONFIG_GFX_SSD1681
#include "ssd1681.c"
#endif

// state control
void gfx_pos(gfx_pos_t newx, gfx_pos_t newy, gfx_align_t newa)
{                               // Set position
   x = newx;
   y = newy;
   a = (newa ? : (GFX_L | GFX_T | GFX_H));
}

static uint32_t gfx_colour_lookup(char c)
{                               // character to colour mapping, default is white
   switch (c)
   {
   case 'k':
   case 'K':
      return BLACK;
#if CONFIG_GFX_BPP >8
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

void gfx_colour(char newf)
{                               // Set foreground
   f_mul = gfx_colour_lookup(f = newf);
}

void gfx_background(char newb)
{                               // Set background
   b_mul = gfx_colour_lookup(b = newb);
}

// State get
gfx_pos_t gfx_x(void)
{
   return x;
}

gfx_pos_t gfx_y(void)
{
   return y;
}

gfx_align_t gfx_a(void)
{
   return a;
}

char gfx_f(void)
{
   return f;
}

char gfx_b(void)
{
   return b;
}

// support
inline void gfx_pixel(gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i)
{                               // set a pixel
   if (gfx_settings.flipxy)
   {
      gfx_pos_t t = x;
      x = y;
      y = t;
   };
   if (gfx_settings.flipx)
      x = CONFIG_GFX_WIDTH - 1 - x;
   if (gfx_settings.flipy)
      y = CONFIG_GFX_HEIGHT - 1 - y;
   if (!gfx || x < 0 || x >= CONFIG_GFX_WIDTH || y < 0 || y >= CONFIG_GFX_HEIGHT)
      return;                   // out of display
#if CONFIG_GFX_BPP <= 8
   const int bits = (1 << CONFIG_GFX_BPP) - 1;
   const int shift = 8 - (x % (8 / CONFIG_GFX_BPP)) - CONFIG_GFX_BPP;
   const int line = CONFIG_GFX_WIDTH * CONFIG_GFX_BPP / 8;
   const int addr = line * y + x * CONFIG_GFX_BPP / 8;
   i >>= (8 - CONFIG_GFX_BPP);
   i &= bits;
   i ^= bits;
   if (((gfx[addr] >> shift) & bits) == i)
      return;
   gfx[addr] = ((gfx[addr] & ~(bits << shift)) | (i << shift));
   gfx_changed = 1;
#else
   uint16_t v = ntohs(f_mul * (i >> (8 - GFX_INTENSITY_BPP)) + b_mul * ((0xFF ^ i) >> (8 - GFX_INTENSITY_BPP)));
   if (v == gfx[(y * CONFIG_GFX_WIDTH) + x])
      return;
   gfx[(y * CONFIG_GFX_WIDTH) + x] = v;
   gfx_changed = 1;
#endif
}

static void gfx_draw(gfx_pos_t w, gfx_pos_t h, gfx_pos_t wm, gfx_pos_t hm, gfx_pos_t * xp, gfx_pos_t * yp)
{                               // move x/y based on drawing a box w/h, set x/y as top left of said box
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

static void gfx_block16(gfx_pos_t x, gfx_pos_t y, gfx_pos_t w, gfx_pos_t h, const uint8_t * data, int l)
{                               // Draw a block from 16 bit greyscale data, l is data width for each row
   if (!l)
      l = (w + 1) / 2;          // default is pixels width
   for (gfx_pos_t row = 0; row < h; row++)
   {
      for (gfx_pos_t col = 0; col < w; col++)
      {
         uint8_t v = data[col / 2];
         gfx_pixel(x + col, y + row, (v & 0xF0) | (v >> 4));
         col++;
         if (col < w)
            gfx_pixel(x + col, y + row, (v & 0xF) | (v << 4));
      }
      data += l;
   }
}

// drawing
void gfx_clear(gfx_intensity_t i)
{
   if (!gfx)
      return;
   for (gfx_pos_t y = 0; y < CONFIG_GFX_HEIGHT; y++)
      for (gfx_pos_t x = 0; x < CONFIG_GFX_WIDTH; x++)
         gfx_pixel(x, y, i);
}

void gfx_set_contrast(gfx_intensity_t contrast)
{
   if (!gfx)
      return;
   gfx_settings.contrast = contrast;
   gfx_update = 1;
   gfx_changed = 1;
}

void gfx_box(gfx_pos_t w, gfx_pos_t h, gfx_intensity_t i)
{                               // draw a box, not filled
   gfx_pos_t x,
    y;
   gfx_draw(w, h, 0, 0, &x, &y);
   for (gfx_pos_t n = 0; n < w; n++)
   {
      gfx_pixel(x + n, y, i);
      gfx_pixel(x + n, y + h - 1, i);
   }
   for (gfx_pos_t n = 1; n < h - 1; n++)
   {
      gfx_pixel(x, y + n, i);
      gfx_pixel(x + w - 1, y + n, i);
   }
}

void gfx_fill(gfx_pos_t w, gfx_pos_t h, gfx_intensity_t i)
{                               // draw a filled rectangle
   gfx_pos_t x,
    y;
   gfx_draw(w, h, 0, 0, &x, &y);
   for (gfx_pos_t row = 0; row < h; row++)
      for (gfx_pos_t col = 0; col < w; col++)
         gfx_pixel(x + col, y + row, i);
}

void gfx_icon16(gfx_pos_t w, gfx_pos_t h, const void *data)
{                               // Icon, 16 bit packed
   if (!data)
      gfx_fill(w, h, 0);        // No icon
   else
   {
      gfx_pos_t x,
       y;
      gfx_draw(w, h, 0, 0, &x, &y);
      gfx_block16(x, y, w, h, data, 0);
   }
}

void gfx_text(int8_t size, const char *fmt, ...)
{                               // Size negative for descenders
   if (!gfx)
      return;
   va_list ap;
   char temp[CONFIG_GFX_WIDTH / 4 + 2];
   va_start(ap, fmt);
   vsnprintf(temp, sizeof(temp), fmt, ap);
   va_end(ap);
   int z = 7;                   // effective height
   if (size < 0)
   {                            // indicates descenders allowed
      size = -size;
      z = 9;
   } else if (!size)
      z = 5;
   if (size > sizeof(fonts) / sizeof(*fonts))
      size = sizeof(fonts) / sizeof(*fonts);
   if (!fonts[size])
      return;
   int fontw = (size ? 6 * size : 4);   // pixel width of characters in font file
   int fonth = (size ? 9 * size : 5);   // pixel height of characters in font file

   int w = 0;                   // width of overall text
   int h = z * (size ? : 1);    // height of overall text
   int cwidth(char c) {         // character width as printed - some characters are done narrow, and <' ' is fixed size move
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
   const uint8_t *fontdata(char c) {
      const uint8_t *d = fonts[size] + (c - ' ') * fonth * fontw / 2;
      if (c == ':' || c == '.')
         d += size;
      //2 pixels in
      return d;
   }
   for (char *p = temp; *p; p++)
      w += cwidth(*p);
   gfx_pos_t x,
    y;
   if (w)
      w -= (size ? : 1);        // Margin right hand pixel needs removing from width
   gfx_draw(w, h, size ? : 1, size ? : 1, &x, &y);      // starting point
   if (!w)
      return;                   // nothing to print
   for (gfx_pos_t n = -1; n <= w; n++)
   {
      gfx_pixel(x + n, y - 1, 0);
      gfx_pixel(x + n, y + h, 0);
   }
   for (gfx_pos_t n = 0; n < h; n++)
   {
      gfx_pixel(x - 1, y + n, 0);
      gfx_pixel(x + w, y + n, 0);
   }
   for (char *p = temp; *p; p++)
   {
      int c = *p;
      int charw = cwidth(c);
      if (charw)
      {
         if (c < ' ')
            c = ' ';
         if (!p[1])
            charw -= (size ? : 1);
         gfx_block16(x, y, charw, h, fontdata(c), fontw / 2);
         x += charw;
      }
   }
}

static void gfx_task(void *p)
{
   const char *e = gfx_driver_init();
   if (e)
   {
      ESP_LOGE(TAG, "Configuration failed %s", e);
      free(gfx);
      gfx = NULL;
      gfx_settings.port = -1;
      vTaskDelete(NULL);
      return;
   }
   gfx_update = 1;
   while (1)
   {                            // Update
      if (!gfx_changed)
      {
         usleep(100000);
         continue;
      }
      gfx_lock();
      gfx_changed = 0;
      gfx_driver_send();
      gfx_unlock();
   }
}

const char *gfx_init_opts(gfx_init_t o)
{                               // Start OLED task and display
   // Defaults
   if (!o.contrast)
      o.contrast = 255;
   if (!o.port)
      o.port = HSPI_HOST;
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
   // Check
   if (!o.mosi)
      return "MOSI not set";
   if (!o.sck)
      return "SCK not set";
   if (!o.dc)
      return "DC not set";
   gfx_settings = o;
   gfx_mutex = xSemaphoreCreateMutex(); // Shared text access
   gfx = malloc(GFX_SIZE);
   if (!gfx)
      return "Mem?";
   memset(gfx, 0, GFX_SIZE);
   spi_bus_config_t config = {
      .mosi_io_num = o.mosi,
      .miso_io_num = -1,
      .sclk_io_num = o.sck,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 8 * (GFX_SIZE + 8),
      .flags = SPICOMMON_BUSFLAG_MASTER,
   };
   if (o.port == HSPI_HOST && o.mosi == 22 && o.sck == 18 && o.cs == 5)
      config.flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
   if (spi_bus_initialize(o.port, &config, 2))
      return "Init?";
   spi_device_interface_config_t devcfg = {
      .clock_speed_hz = SPI_MASTER_FREQ_20M,
      .mode = 0,
      .spics_io_num = o.cs,
      .queue_size = 1,
      .flags = SPI_DEVICE_3WIRE,
   };
   if (spi_bus_add_device(o.port, &devcfg, &gfx_spi))
      return "Add?";
   gpio_set_direction(o.dc, GPIO_MODE_OUTPUT);
   if (o.rst)
   {
      gpio_set_direction(o.rst, GPIO_MODE_OUTPUT);
      gpio_set_level(o.rst, 1);
      usleep(100000);
      gpio_set_level(o.rst, 0);
      usleep(100000);
   }
   if (o.ena)
      gpio_set_level(o.ena, 1); // Enable
   xTaskCreate(gfx_task, "OLED", 8 * 1024, NULL, 2, &gfx_task_id);
   return NULL;
}

void gfx_lock(void)
{                               // Lock display task
   if (gfx_mutex)
      xSemaphoreTake(gfx_mutex, portMAX_DELAY);
   gfx_locks++;
   // preset state
   gfx_background('k');
   gfx_colour('w');
   gfx_pos(0, 0, GFX_L | GFX_T | GFX_H);
}

void gfx_unlock(void)
{                               // Unlock display task
   gfx_locks--;
   if (gfx_mutex)
      xSemaphoreGive(gfx_mutex);
}
