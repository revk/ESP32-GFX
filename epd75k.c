// Waveshare EPD75 (7.5" e-paper) driver
// https://files.waveshare.com/upload/6/60/7.5inch_e-Paper_V2_Specification.pdf
// https://www.waveshare.com/w/upload/8/8c/7.5inch-e-paper-b-v3-specification.pdf
// https://files.waveshare.com/upload/8/87/E-Paper-Driver-HAT-Schematic.pdf
// https://files.waveshare.com/upload/8/8e/E-Paper_Driver_HAT.pdf
// Also https://github.com/bitbank2/OneBitDisplay/blob/5d3d41b6de167f7bc51228f4710f78a31b6c8002/src/obd.inl#L2842
// Also https://benkrasnow.blogspot.com/2017/10/fast-partial-refresh-on-42-e-paper.html

#define GFX_DEFAULT_WIDTH	800
#define GFX_DEFAULT_HEIGHT	480
#define GFX_BPP			1
#define	GFX_BUSY_LOW

#ifndef	CONFIG_REVK_APPNAME
const uint8_t epdslow = 0;      // Default slow update LUT
const uint8_t epdsleep = 0;     // Deep sleep
const uint8_t epdamv = 1;       // Send AMV
const uint8_t epdpfs = 1;       // Send PSR
const uint8_t epdevs = 1;       // Send EDVS
const uint8_t epdrepeat = 1;    // Additional fast update
const int8_t epdtse = 0x80;     // Default tse when no settings.def used
#endif

#define	EPD75_PSR	0x00
#define	EPD75_PWR	0x01
#define	EPD75_POF	0x02
#define	EPD75_PFS	0x03
#define	EPD75_PON	0x04
#define	EPD75_PMES	0x05
#define	EPD75_BTST	0x06
#define	EPD75_DSLP	0x07
#define	EPD75_DTM1	0x10
#define	EPD75_DSP	0x11
#define	EPD75_DRF	0x12
#define	EPD75_DTM2	0x13
#define	EPD75_DSPI	0x15
#define	EPD75_AUTO	0x17
#define	EPD75_LUT_VCOM	0x20
#define	EPD75_LUT_WW	0x21
#define	EPD75_LUT_KW	0x22
#define	EPD75_LUT_WK	0x23
#define	EPD75_LUT_KK	0x24
#define	EPD75_LUT_VCOM2	0x25
#define	EPD75_KWOPT	0x2B
#define	EPD75_PLL	0x30
#define	EPD75_TSC	0x40
#define	EPD75_TSE	0x41
#define	EPD75_TSW	0x42
#define	EPD75_TSR	0x43
#define	EPD75_PBC	0x44
#define	EPD75_CDI	0x50
#define	EPD75_LPD	0x51
#define	EPD75_EVS	0x52
#define	EPD75_TCON	0x60
#define	EPD75_TRES	0x61
#define	EPD75_GSST	0x65
#define	EPD75_REV	0x70
#define	EPD75_FLG	0x71
#define	EPD75_AMV	0x80
#define	EPD75_VV	0x81
#define	EPD75_VDCS	0x82
#define	EPD75_PTL	0x90
#define	EPD75_PTIN	0x91
#define	EPD75_PTOUT	0x92
#define	EPD75_PGM	0xA0
#define	EPD75_APG	0xA1
#define	EPD75_ROTP	0xA2
#define	EPD75_CCSET	0xE0
#define	EPD75_PWS	0xE3
#define	EPD75_LVSEL	0xE4
#define	EPD75_TSSET	0xE5
#define	EPD75_TSBDRY	0xE7

#include <driver/rtc_io.h>

#define               USE_AUTO  // Auto PON/DRF/POF sequence
//#define       USE_N2OCP       // Auto copy buffer (seems not to work)

#define	T1	30
#define	T2	5
#define	T3	30
#define	T4	5
#define	T5	10
#define	T6	5
#define	T7	10
#define	T8	5
#define	WW	0x22	// 0x02
#define	KW	0x48
#define	WK	0x84
#define	KK	0x11	// 0x04

extern uint32_t uptime (void);

static uint8_t lut = 0;

static void
fastlut (void)
{
   if (!epdsleep && lut == 1)
      return;
   lut = 1;
   ESP_LOGD (TAG, "Fast LUT");
   // My LUT
   const uint8_t lut[] = {
      43, EPD75_LUT_VCOM,       // LUT (7 groups as no red)
      0x00, T1, T2, T3, T4, 1,
      0x00, T5, T6, T7, T8, epdrepeat,
      //0x00, T5, T6, T7, 0, 1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      43, EPD75_LUT_WW,
      WW, T1, T2, T3, T4, 1,
      WW, T5, T6, T7, T8, epdrepeat,
      //0x02, T5, T6, T7, 0, 1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      43, EPD75_LUT_KW,
      KW, T1, T2, T3, T4, 1,
      WW, T5, T6, T7, T8, epdrepeat,
      //0x02, T5, T6, T7, 0, 1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      43, EPD75_LUT_WK,
      WK, T1, T2, T3, T4, 1,
      KK, T5, T6, T7, T8, epdrepeat,
      //0x04, T5, T6, T7, 0, 1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      43, EPD75_LUT_KK,
      KK, T1, T2, T3, T4, 1,
      KK, T5, T6, T7, T8, epdrepeat,
      //0x04, T5, T6, T7, 0, 1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
#if 1
      43, EPD75_LUT_VCOM2,
      0x00, T1, T2, T3, T4, 1,
      0x00, T5, T6, T7, T8, epdrepeat,
      //0x00, T5, T6, T7, 0, 1,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
#endif
      0
   };
   gfx_command_bulk (lut);
}

static void
slowlut (void)
{                               // slow (flashy) update 
   if (!epdsleep && lut == 2)
      return;
   lut = 2;
   ESP_LOGD (TAG, "Slow LUT");
   // Waveshare example LUT
   const uint8_t lut[] = {
      43, EPD75_LUT_VCOM,       // LUT (7 groups as no red)
      0x0, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x0, 0xF, 0x1, 0xF, 0x1, 0x2,
      0x0, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      43, EPD75_LUT_WW,
      0x10, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x84, 0xF, 0x1, 0xF, 0x1, 0x2,
      0x20, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      43, EPD75_LUT_KW,
      0x10, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x84, 0xF, 0x1, 0xF, 0x1, 0x2,
      0x20, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      43, EPD75_LUT_WK,
      0x80, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x84, 0xF, 0x1, 0xF, 0x1, 0x2,
      0x40, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      43, EPD75_LUT_KK,
      0x80, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x84, 0xF, 0x1, 0xF, 0x1, 0x2,
      0x40, 0xF, 0xF, 0x0, 0x0, 0x1,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
      0
   };
   gfx_command_bulk (lut);
}

static const char *
gfx_driver_init (void)
{                               // Initialise
   uint64_t a = esp_timer_get_time ();
   int W = gfx_settings.width;  // Must be multiple of 8
   int H = gfx_settings.height;
   //if(!epdsleep)gfx_command1 (EPD75_PSR, 0x00);      // Reset

   const uint8_t init[] = {
#if 0
      // Waveshare example exactly
      6, EPD75_PWR, 0x17, 0x17, 0x3F, 0x3F, 0x11,       //
      2, EPD75_VDCS, 0x24,      //
      5, EPD75_BTST, 0x27, 0x27, 0x2F, 0x17,    //
      2, EPD75_PLL, 0x06,       //
      1, EPD75_PON,
      0xFF,
      2, EPD75_PSR, 0x3F,       // Use REG
      5, EPD75_TRES, W / 256, W & 255, H / 256, H & 255,        //
      2, EPD75_DSPI, 0x00,      //
      3, EPD75_CDI, 0x10, 0x00, //
      2, EPD75_TCON, 0x22,      // 
      5, EPD75_GSST, 0, 0, 0, 0,        // waveshare and esphome send this
#else
      // My settings
      5, EPD75_PWR, 0x07, 0x17, 0x3A, 0x3A,     // or 17
      2, EPD75_VDCS, 0x26,      //
      5, EPD75_BTST, 0x17, 0x17, 0x27, 0x17,
      2, EPD75_PLL, 0x06,       //
      1, EPD75_PON,
      0xFF,
      2, EPD75_PSR, 0x3F,       // Use REG
      5, EPD75_TRES, W / 256, W & 255, H / 256, H & 255,        //
      2, EPD75_DSPI, 0x00,      //
      3, EPD75_CDI, 0x10, 0x00, //
      2, EPD75_TCON, 0x22,      //
      5, EPD75_GSST, 0, 0, 0, 0,        // waveshare and esphome send this
#endif
      0
   };
   if (gfx_command_bulk (init))
      return "Init1 failed";
   if (epdtse)
      gfx_command1 (EPD75_TSE, epdtse & 0xF);   // Temp compensation -8 to +7
   if (epdamv && !epdsleep)
      gfx_command1 (EPD75_AMV, 0x11);   // VCOM cal
   if (epdpfs)
      gfx_command1 (EPD75_PFS, 0x30);   // Power down time
   if (epdevs)
      gfx_command1 (EPD75_EVS, 0x08);   // End voltage (floating)
   if (epdslow)
      fastlut ();               // We leave the LUT as the fast one rather than change each time
   gfx_command0 (EPD75_POF);    // Needed as we did PON
   gfx_busy_wait ();
   uint64_t b = esp_timer_get_time ();
   ESP_LOGD (TAG, "Init time %lldms", (b - a + 500) / 1000);
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   if (!epdsleep)
      return NULL;
   if (gfx_command0 (EPD75_DSLP))
      return "DSLP failed";
   gfx_settings.asleep = 1;
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   uint64_t a = esp_timer_get_time ();
   static uint32_t waiting = 0;
   if (gfx_settings.asleep)
   {
      if (waiting)
      {
         uint32_t up = uptime ();
         if (waiting > up)
            sleep (waiting - up);
         waiting = 0;
      }
      gfx_settings.asleep = 0;
      gpio_set_level (gfx_settings.rst, 0);
      usleep (10000);
      gpio_set_level (gfx_settings.rst, 1);
      usleep (10000);
      gfx_driver_init ();
   }

   if (epdslow)
      gfx_command1 (EPD75_PSR, gfx_settings.norefresh ? 0x3F : 0x1F);   //  KW, LUT=REG (fast update) or LUT=OTP (slow), dir could be used for flip, 
   else
   {                            // Always REG, switch LUT as needed
      if (gfx_settings.norefresh)
         fastlut ();
      else
         slowlut ();
   }

   gfx_command2 (EPD75_CDI,
#ifdef	USE_N2OCP
                 8 |
#endif
                 (gfx_settings.norefresh ? 0x80 : 0x00) |       // Border if refresh
                 ((gfx_settings.border ^ gfx_settings.invert) ? 0x10 : 0x20) |  // border colour
                 0x01,          // new+old logic refresh
                 0x07);

   if (epdsleep)
   {                            // Send OLD
#define SIZE (GFX_DEFAULT_WIDTH/8*GFX_DEFAULT_HEIGHT)
      static uint8_t *old = NULL;
      if (!old)
      {
         old = malloc (SIZE);
         if (old)
            memset (old, 0, SIZE);
         else
            ESP_LOGE (TAG, "Cannot malloc old");
      }
      if (old)
      {
         if (gfx_command0 (EPD75_DTM1) || gfx_send_data (old, SIZE))
            return "DTM1 failed";
         memcpy (old, gfx_raw_b (), SIZE);
      }
#undef SIZE
   }

   if (gfx_command0 (EPD75_DTM2) || gfx_send_gfx (0))   // Send NEW
      return "DTM2 failed";

#ifdef	USE_AUTO
   if (epdsleep)
   {
      if (gfx_command1 (EPD75_AUTO, 0xA7))      // PON->DRF->POF->DSLP
         return "AUTO+DSLP failed";
      if (gfx_settings.norefresh)
      {                         // Reset to try and avoid fading
         sleep (4);
         gpio_set_level (gfx_settings.rst, 0);
         usleep (10000);
         gpio_set_level (gfx_settings.rst, 1);
         usleep (10000);
      } else
         waiting = uptime () + 10;
      gfx_settings.asleep = 1;
   } else
   {
      if (gfx_command1 (EPD75_AUTO, 0xA5))      // PON->DRF->POF
         return "AUTO failed";
      gfx_busy_wait ();
   }
#else // Not auto
   if (gfx_command0 (EPD75_PON))
      return "PON failed";
   if (gfx_command0 (EPD75_DRF))
      return "DRF failed";
   gfx_command0 (EPD75_POF);
   gfx_driver_sleep ();         // Only sleeps if we are using DSLP
#endif
   if (!epdsleep)
   {
      if (gfx_command0 (EPD75_DTM1) || gfx_send_gfx (0))
         return "DTM1 failed";
   }
   uint64_t b = esp_timer_get_time ();
   ESP_LOGD (TAG, "Draw time %lldms%s", (b - a + 500) / 1000, gfx_settings.asleep ? " (sleep)" : "");
   return NULL;
}
