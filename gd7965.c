// GD7965 driver code
// https://files.waveshare.com/upload/6/60/7.5inch_e-Paper_V2_Specification.pdf

#define GFX_DEFAULT_WIDTH	800
#define GFX_DEFAULT_HEIGHT	480
#define GFX_BPP			1
#define	GFX_BUSY_LOW
//#define	GFX_INVERT

#define	GD7965_PSR	0x00
#define	GD7965_PWR	0x01
#define	GD7965_POF	0x02
#define	GD7965_PFS	0x03
#define	GD7965_PON	0x04
#define	GD7965_PMES	0x05
#define	GD7965_BTST	0x06
#define	GD7965_DSLP	0x07
#define	GD7965_DTM1	0x10
#define	GD7965_DSP	0x11
#define	GD7965_DRF	0x12
#define	GD7965_DTM2	0x13
#define	GD7965_DSPI	0x15
#define	GD7965_AUTO	0x17
#define	GD7965_KWOPT	0x2B
#define	GD7965_PLL	0x30
#define	GD7965_TSC	0x40
#define	GD7965_TSE	0x41
#define	GD7965_TSW	0x42
#define	GD7965_TSR	0x43
#define	GD7965_PBC	0x44
#define	GD7965_CDI	0x50
#define	GD7965_LPD	0x51
#define	GD7965_EVS	0x52
#define	GD7965_TCON	0x60
#define	GD7965_TRES	0x61
#define	GD7965_GSST	0x65
#define	GD7965_REV	0x70
#define	GD7965_FLG	0x71
#define	GD7965_AMV	0x80
#define	GD7965_VV	0x81
#define	GD7965_VDCS	0x82
#define	GD7965_PTL	0x90
#define	GD7965_PTIN	0x91
#define	GD7965_PTOUT	0x92
#define	GD7965_PGM	0xA0
#define	GD7965_APG	0xA1
#define	GD7965_ROTP	0xA2
#define	GD7965_CCSET	0xE0
#define	GD7965_PWS	0xE3
#define	GD7965_LVSEL	0xE4
#define	GD7965_TSSET	0xE5
#define	GD7965_TSBDRY	0xE7

#include <driver/rtc_io.h>

#define	USE_AUTO		// Auto PON/POFF sequence
#define	USE_DSLP		// Deep sleep

static const char *
gfx_driver_init (void)
{                               // Initialise
   ESP_LOGE (TAG, "Init");
   int W = gfx_settings.width;  // Must be multiple of 8
   int H = gfx_settings.height;
   const uint8_t ssd1681_default_init_code[] = {
      //0xFF, 0,                 // busy wait
      GD7965_BTST, 4, 0x17, 0x17, 0x27, 0x17,   //
      GD7965_PWR, 4, 0x07, 0x17, 0x3F, 0x3F,    // Data sheet says 5 fields, but example and other drivers say 4
#ifndef	USE_AUTO
      GD7965_PON, 0,            //
      0xFF, 0,                  // busy wait
#endif
      //GD7965_PSR, 1, 0x3F,      // KW LUT=REG
      GD7965_PSR, 1, 0x1F,      // KW LUT=OTP
      GD7965_PLL, 1, 0x06,      //
      GD7965_TRES, 4, W / 256, W & 255, H / 256, H & 255,       //
      GD7965_DSPI, 1, 0x00,     //
      GD7965_TCON, 1, 0x22,     //
      GD7965_VDCS, 1, 0x26,     //
      //GD7965_CDI, 2, 0x31, 0x07,        //
      GD7965_CDI, 2, 0x29, 0x07,        // N2OCP (copy new to old on refresh) (white border)
      0xFE                      // End
   };
   if (gfx_command_list (ssd1681_default_init_code))
      return "Init failed";
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
#ifdef	USE_DSLP
   gpio_set_level (gfx_settings.rst, 0);
   usleep (1000);
   gpio_set_level (gfx_settings.rst, 1);
   usleep (1000);
   gfx_driver_init();
#endif
   if (gfx_send_command (GD7965_DTM2))
      return "DTM2 failed";
   if (gfx_send_gfx ())
      return "Data send failed";
#ifdef	USE_AUTO
#ifdef	USE_DSLP
   if (gfx_command1 (GD7965_AUTO, 0xA7))        // PON->DRF->POFF->DSLP
#else
   if (gfx_command1 (GD7965_AUTO, 0xA5))        // PON->DRF->POFF
#endif
      return "AUTO failed";
#else
   if (gfx_send_command (GD7965_DRF))
      return "DRF failed";
#endif
   gfx_busy_wait ("Post draw");
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   return NULL;
}
