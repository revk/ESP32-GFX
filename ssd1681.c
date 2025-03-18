// SSD1681 driver code
// http://www.e-paper-display.com/download_detail/downloadsId=825.html

#define GFX_DEFAULT_WIDTH	200
#define GFX_DEFAULT_HEIGHT	200
#define GFX_BPP			1

#define SSD1681_DRIVER_CONTROL 0x01
#define SSD1681_GATE_VOLTAGE 0x03
#define SSD1681_SOURCE_VOLTAGE 0x04
#define SSD1681_PROGOTP_INITIAL 0x08
#define SSD1681_PROGREG_INITIAL 0x09
#define SSD1681_READREG_INITIAL 0x0A
#define SSD1681_BOOST_SOFTSTART 0x0C
#define SSD1681_DEEP_SLEEP 0x10
#define SSD1681_DATA_MODE 0x11
#define SSD1681_SW_RESET 0x12
#define SSD1681_TEMP_CONTROL 0x18
#define SSD1681_TEMP_WRITE 0x1A
#define SSD1681_MASTER_ACTIVATE 0x20
#define SSD1681_DISP_CTRL1 0x21
#define SSD1681_DISP_CTRL2 0x22
#define SSD1681_WRITE_RAM1 0x24
#define SSD1681_WRITE_RAM2 0x26
#define SSD1681_WRITE_VCOM 0x2C
#define SSD1681_READ_OTP 0x2D
#define SSD1681_READ_STATUS 0x2F
#define SSD1681_WRITE_LUT 0x32
#define SSD1681_WRITE_BORDER 0x3C
#define SSD1681_SET_RAMXPOS 0x44
#define SSD1681_SET_RAMYPOS 0x45
#define SSD1681_SET_RAMXCOUNT 0x4E
#define SSD1681_SET_RAMYCOUNT 0x4F

#include <driver/rtc_io.h>
RTC_NOINIT_ATTR uint64_t sleepms;       // Sleeping in low power, needs delay before reset after mode 2

static const char *
gfx_driver_init (void)
{                               // Initialise
   ESP_LOGD (TAG, "Init");
   int W = gfx_settings.width;
   int H = gfx_settings.height;
   const uint8_t init[] = {
      1, SSD1681_SW_RESET,      // soft reset
      0xFF,                     // busy wait
      4, SSD1681_DRIVER_CONTROL, (H - 1), (H - 1) >> 8, 0,      //
      2, SSD1681_DATA_MODE, 0x03,       // Ram data entry mode
      2, SSD1681_WRITE_BORDER, 0x05,    // border color
      2, SSD1681_TEMP_CONTROL, 0x80,    // Temp control
      3, SSD1681_SET_RAMXPOS, 0, (W + 7) / 8 - 1,       //
      5, SSD1681_SET_RAMYPOS, 0, 0, (H - 1) & 0xFF, (H - 1) / 256,      //
      0
   };

   if (gfx_command_bulk (init))
      return "Init failed";
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   if (gfx_settings.asleep && gfx_settings.rst)
   {                            // Needs a reset
      if (sleepms)
      {
         struct timeval tv;
         gettimeofday (&tv, NULL);
         uint64_t ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
         ms -= sleepms;
         if (ms < 1000)
            return "Wait";
         sleepms = 0;
      }
      ESP_LOGD (TAG, "Reset");
      gfx_settings.asleep = 0;
      gpio_set_level (gfx_settings.rst, 0);
      usleep (1000);
      gpio_set_level (gfx_settings.rst, 1);
      usleep (1000);
   }
   if (gfx_command1 (SSD1681_SET_RAMXCOUNT, 0))
      return "Set X failed";
   if (gfx_command2 (SSD1681_SET_RAMYCOUNT, 0, 0))
      return "Set Y failed";
   if (gfx_command0 (SSD1681_WRITE_RAM1))
      return "Data start failed";
   if (gfx_send_gfx (0))
      return "Data send failed";

   if (gfx_settings.norefresh && gfx_settings.mode2)
   {                            // mode 2
      ESP_LOGD (TAG, "Mode2");
      if (gfx_command1 (SSD1681_DISP_CTRL2, 0xFF))
         return "Display ctrl failed";
      if (gfx_command0 (SSD1681_MASTER_ACTIVATE))
         return "Master activate failed";
      if (gfx_settings.sleep)
      {
         gfx_settings.asleep = 1;
         struct timeval tv;
         gettimeofday (&tv, NULL);
         sleepms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
         // If we wait here, even wait busy as you would expect, we actually get a flicker display. Sending sleep right away works way better, silly
         if (gfx_command1 (SSD1681_DEEP_SLEEP, 1))
            return "Sleep fail";
      } else
         sleepms = 0;
   } else
   {                            // mode 1
      sleepms = 0;
      ESP_LOGD (TAG, "Mode1");
      if (gfx_command1 (SSD1681_DISP_CTRL2, 0xF7))
         return "Display ctrl failed";
      if (gfx_command0 (SSD1681_MASTER_ACTIVATE))
         return "Master activate failed";
      if (gfx_settings.mode2)
      {                         // Will revert
         if (gfx_command1 (SSD1681_DISP_CTRL2, 0xFF))
            return "Display ctrl failed";
         if (gfx_command0 (SSD1681_MASTER_ACTIVATE))
            return "Master activate failed";
      }
   }
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   if (gfx_settings.asleep)
      return NULL;
   gfx_settings.sleep = 1;
   struct timeval tv;
   gettimeofday (&tv, NULL);
   sleepms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
   ESP_LOGD (TAG, "Sleep");
   gfx_settings.asleep = 1;
   if (gfx_command1 (SSD1681_DEEP_SLEEP, 1))
      return "Sleep fail";
   return NULL;

}
