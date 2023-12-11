// Waveshae 2.9" 128x296 1bpp (black/white) driver code
// https://files.waveshare.com/upload/e/e6/2.9inch_e-Paper_Datasheet.pdf
// This is basically the same as SSD1681

#define GFX_DEFAULT_WIDTH	128
#define GFX_DEFAULT_HEIGHT	296
#define GFX_BPP			1

#define EPD29_DRIVER_CONTROL 0x01
#define EPD29_GATE_VOLTAGE 0x03
#define EPD29_SOURCE_VOLTAGE 0x04
#define EPD29_PROGOTP_INITIAL 0x08
#define EPD29_PROGREG_INITIAL 0x09
#define EPD29_READREG_INITIAL 0x0A
#define EPD29_BOOST_SOFTSTART 0x0C
#define EPD29_DEEP_SLEEP 0x10
#define EPD29_DATA_MODE 0x11
#define EPD29_SW_RESET 0x12
#define EPD29_TEMP_CONTROL 0x18
#define EPD29_TEMP_WRITE 0x1A
#define EPD29_MASTER_ACTIVATE 0x20
#define EPD29_DISP_CTRL1 0x21
#define EPD29_DISP_CTRL2 0x22
#define EPD29_WRITE_RAM1 0x24
#define EPD29_WRITE_RAM2 0x26
#define EPD29_WRITE_VCOM 0x2C
#define EPD29_READ_OTP 0x2D
#define EPD29_READ_STATUS 0x2F
#define EPD29_WRITE_LUT 0x32
#define EPD29_DUMMY_LINE_PERIOD 0x3A
#define EPD29_SET_GATE_TIME 0x3B
#define EPD29_WRITE_BORDER 0x3C
#define EPD29_SET_RAMXPOS 0x44
#define EPD29_SET_RAMYPOS 0x45
#define EPD29_SET_RAMXCOUNT 0x4E
#define EPD29_SET_RAMYCOUNT 0x4F

#include <driver/rtc_io.h>

static const char *
gfx_driver_init (void)
{                               // Initialise
   ESP_LOGD (TAG, "Init");
   int W = gfx_settings.width;
   int H = gfx_settings.height;
   const uint8_t init[] = {
      1, EPD29_SW_RESET,       // soft reset
      0xFF,                     // busy wait
      4, EPD29_DRIVER_CONTROL, (H - 1) & 0xFF, (H - 1) >> 8, 0,        //
      4, EPD29_BOOST_SOFTSTART, 0xD7, 0xD6, 0x9D,      // 
      2, EPD29_WRITE_VCOM, 0xA8,       // 
      2, EPD29_DUMMY_LINE_PERIOD, 0x1A,        //
      2, EPD29_SET_GATE_TIME, 0x08,    //
      2, EPD29_DATA_MODE, 0x03,        // Ram data entry mode
      3, EPD29_SET_RAMXPOS, 0, (W + 7) / 8-1,        //
      5, EPD29_SET_RAMYPOS, 0, 0, (H - 1) & 0xFF, (H - 1) >> 8,        //
      31, EPD29_WRITE_LUT,     //
          0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Full update
      2, EPD29_TEMP_CONTROL, 0x80,     // Temp control ?
      0
   };
   if (gfx_command_bulk (init))
      return "Init failed";
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   const uint8_t init[] = {
      2, EPD29_WRITE_BORDER, 0x05,     // border color (TODO)
      // TODO LUT for fast refresh?
      2, EPD29_SET_RAMXCOUNT, 0,
      3, EPD29_SET_RAMYCOUNT, 0, 0,
      0
   };
   if (gfx_command_bulk (init))
      return "Init failed";
   gfx_send_command (EPD29_WRITE_RAM1);
   gfx_send_gfx (0);
   gfx_command1 (EPD29_DISP_CTRL2, 0xF7);
   gfx_send_command (EPD29_MASTER_ACTIVATE);
   gfx_send_command (0xFF);
   gfx_busy_wait ("send");
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   return NULL;
}
