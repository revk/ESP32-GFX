// GD7965 driver code
// https://files.waveshare.com/upload/6/60/7.5inch_e-Paper_V2_Specification.pdf

#define GFX_DEFAULT_WIDTH	800
#define GFX_DEFAULT_HEIGHT	480
#define GFX_BPP			1

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

static const char *
gfx_driver_init (void)
{                               // Initialise
   ESP_LOGE (TAG, "Init");
   int W = gfx_settings.width;  // Must be multiple of 8
   int H = gfx_settings.height;
   const uint8_t ssd1681_default_init_code[] = {
      GD7965_BTST, 4, 0x17, 0x17, 0x27, 0x17,   //
      GD7965_PWR, 5, 0x07, 0x17, 0x3F, 0x3F, 0x03,      //
      GD7965_PON, 0,            //
      0xFF, 20,                 // busy wait
      GD7965_PSR, 1, 0x3F,      //
      GD7965_PLL, 1, 0x06,      //
      GD7965_TRES, 4, W / 256, W & 255, H / 256, H & 255,       //
      GD7965_DSPI, 1, 0x00,     //
      GD7965_TCON, 1, 0x22,     //
      GD7965_VDCS, 1, 0x26,     //
      //GD7965_CDI, 2, 0x31, 0x07,        //
      GD7965_CDI, 2, 0x39, 0x07,        // N2OCP (copy new to old on refresh)
      0xFE                      // End
   };
   if (gfx_command_list (ssd1681_default_init_code))
      return "Init failed";
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   gfx_busy_wait ("Pre draw");
   if (gfx_send_command (GD7965_DTM2))
      return "Data start 2 failed";
   if (gfx_send_gfx ())
      return "Data send 2 failed";
   if (gfx_send_command (GD7965_DRF))
      return "Data start 2 failed";
   gfx_busy_wait ("Post draw");
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   return NULL;
}
