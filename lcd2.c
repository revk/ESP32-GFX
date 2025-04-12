// Waveshare 2" ST7789V
// https://github.com/waveshareteam/waveshare_fbcp/tree/main/src/lcd_driver

#define GFX_DEFAULT_WIDTH	320
#define GFX_DEFAULT_HEIGHT	240
#define GFX_BPP			16
//#define       GFX_FLIP_XY     // 90 degree flip in hardware
#define	GFX_FLIP_X              // X flip in hardware
#define	GFX_FLIP_Y              // Y fli in hardware

static const char *
gfx_driver_init (void)
{                               // Initialise
   gfx_command0 (0x11); // Sleep out
   usleep (120000);
   gfx_command1 (0x3A,0x05); // 16 bpp
   usleep (120000);
   const uint8_t init[] = {
	   1,0x20, // No invert
	   1,0x13, //
      0
   };
   if (gfx_command_bulk (init))
      return "Init1 failed";
   gfx_command1 (0x36, 0x08
#ifdef	GFX_FLIP_XY
                 + (gfx_settings.flip & 4 ? 0x20 : 0)
#endif
#ifdef	GFX_FLIP_Y
                 + (gfx_settings.flip & 2 ? 0x80 : 0)
#endif
#ifdef	GFX_FLIP_X
                 + (gfx_settings.flip & 1 ? 0x40 : 0)
#endif
      );                        // bit3:RGB, bit5:rowcolswap, bit6:colrev, bit7:rowrev
#ifdef	GFX_FLIP_XY
   if(gfx_settings.flip & 4)gfx_command1(,0x27,320-GFX_DEFAULT_HEIGHT);
#endif
   usleep (10000);
   gfx_command0 (0x29);         // Display on
   if (gfx_settings.bl)
      gpio_set_level (gfx_settings.bl, 1);
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   if (gfx_settings.bl)
      gpio_set_level (gfx_settings.bl, 0);
   gfx_command0 (0x28);         // Display off
   gfx_command0 (0x10);         // Sleep
   usleep (120000);
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   gfx_command0 (0x2C);
   gfx_send_gfx (0);
   return NULL;
}
