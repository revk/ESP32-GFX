// Waveshae 1.54" 200x200 1bpp (black/red/white) driver code
// https://files.waveshare.com/upload/e/e5/1.54inch_e-paper_V2_Datasheet.pdf
// https://files.waveshare.com/upload/9/9e/1.54inch-e-paper-b-v2-specification.pdf
// https://github.com/waveshareteam/e-Paper/blob/master/Arduino/epd1in54_V2/epd1in54_V2.cpp

#define GFX_DEFAULT_WIDTH	200
#define GFX_DEFAULT_HEIGHT	200
#define GFX_BPP			2

#define EPD154_DRIVER_CONTROL 0x01
#define EPD154_GATE_VOLTAGE 0x03
#define EPD154_SOURCE_VOLTAGE 0x04
#define EPD154_PROGOTP_INITIAL 0x08
#define EPD154_PROGREG_INITIAL 0x09
#define EPD154_READREG_INITIAL 0x0A
#define EPD154_BOOST_SOFTSTART 0x0C
#define EPD154_DEEP_SLEEP 0x10
#define EPD154_DATA_MODE 0x11
#define EPD154_SW_RESET 0x12
#define EPD154_TEMP_CONTROL 0x18
#define EPD154_TEMP_WRITE 0x1A
#define EPD154_MASTER_ACTIVATE 0x20
#define EPD154_DISP_CTRL1 0x21
#define EPD154_DISP_CTRL2 0x22
#define EPD154_WRITE_RAM1 0x24
#define EPD154_WRITE_RAM2 0x26
#define EPD154_WRITE_VCOM 0x2C
#define EPD154_READ_OTP 0x2D
#define EPD154_READ_STATUS 0x2F
#define EPD154_WRITE_LUT 0x32
#define EPD154_DUMMY_LINE_PERIOD 0x3A
#define EPD154_SET_GATE_TIME 0x3B
#define EPD154_WRITE_BORDER 0x3C
#define EPD154_SET_RAMXPOS 0x44
#define EPD154_SET_RAMYPOS 0x45
#define EPD154_SET_RAMXCOUNT 0x4E
#define EPD154_SET_RAMYCOUNT 0x4F

#include <driver/rtc_io.h>

static const char *
gfx_driver_init (void)
{                               // Initialise
   ESP_LOGD (TAG, "Init");
   int W = gfx_settings.width;
   int H = gfx_settings.height;
   const uint8_t init[] = {
      1, EPD154_SW_RESET,       // soft reset
      4, EPD154_DRIVER_CONTROL, (H - 1) & 0xFF, (H - 1) >> 8, 0,        //
      4, EPD154_BOOST_SOFTSTART, 0xD7, 0xD6, 0x9D,      // 
      2, EPD154_WRITE_VCOM, 0x7C,       // 
      2, EPD154_DUMMY_LINE_PERIOD, 0xA8,        //
      2, EPD154_SET_GATE_TIME, 0x08,    //
      2, EPD154_DATA_MODE, 0x03,        // Ram data entry mode
      3, EPD154_SET_RAMXPOS, 0, (W + 7) / 8 - 1,        //
      5, EPD154_SET_RAMYPOS, 0, 0, (H - 1) & 0xFF, (H - 1) >> 8,        //
      //31, EPD154_WRITE_LUT,     //
      //0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00,       // full update
      2, EPD154_TEMP_CONTROL, 0x80,     // Temp control ?
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
      2, EPD154_WRITE_BORDER, gfx_settings.border == 0 ? 0x05 : gfx_settings.border == 1 ? 0x04 : 0x00, // border color (TODO no refresh?) TODO red not working yet
      2, EPD154_SET_RAMXCOUNT, 0,
      3, EPD154_SET_RAMYCOUNT, 0, 0,
      0
   };
   if (gfx_command_bulk (init))
      return "Init failed";
   gfx_send_command (EPD154_WRITE_RAM1);
   gfx_send_gfx (0);
   gfx_send_command (EPD154_WRITE_RAM2);
   gfx_send_gfx (1);
   gfx_command1 (EPD154_DISP_CTRL2, 0xF7);
   gfx_send_command (EPD154_MASTER_ACTIVATE);
   gfx_send_command (0xFF);
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   return NULL;
}
