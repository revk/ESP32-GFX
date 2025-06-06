// Waveshare 2.4" ILI9341
// https://github.com/waveshareteam/waveshare_fbcp/tree/main/src/lcd_driver

#define GFX_DEFAULT_WIDTH	240
#define GFX_DEFAULT_HEIGHT	320
#define GFX_BPP			16
#define GFX_FLIP_XY             // 90 degree flip in hardware
#define	GFX_FLIP_X              // X flip in hardware
#define	GFX_FLIP_Y              // Y fli in hardware

#define ILI9341_FRAMERATE_119_HZ 0x10

#define ILI9341_UPDATE_FRAMERATE ILI9341_FRAMERATE_119_HZ

#define ILI9341_PUMP_CONTROL_3XVCI 0x30
#define ILI9341_PUMP_CONTROL ILI9341_PUMP_CONTROL_3XVCI

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   gfx_command0 (0x2C);
   gfx_send_gfx (0);
   return NULL;
}

static const char *
gfx_driver_init (void)
{                               // Initialise
   gfx_command0 (0x01);         // Reset
   usleep (5000);
   const uint8_t init[] = {
      1, 0x28,                  // Off
      6, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,    // These are the same as power on.
      4, 0xCF, 0x00, 0xC1, 0x30,        // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
      4, 0xE8, 0x85, 0x00, 0x78,        // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
      3, 0xEA, 0x00, 0x00,      // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
      5, 0xED, 0x64, 0x03, 0x12, 0x81,  // Not sure what the effect is, set to default as per ILI9341 Application Notes v0.6 (2011/03/11) document (which is not apparently same as default at power on).
      2, 0xF7, ILI9341_PUMP_CONTROL,    //
      2, 0xC0, 0x23,            // Set the GVDD level, which is a reference level for the VCOM level and the grayscale voltage level.
      2, 0xC1, 0x10,            // Sets the factor used in the step-up circuits. To reduce power consumption, set a smaller factor.
      3, 0xC5, 0x3E, 0x28,      // Adjusting VCOM 1 and 2 can control display brightness
      2, 0xC7, 0x86,            //
#ifdef	GFX_FLIP_XY
      5, 0x2A, 0, 0, gfx_settings.width >> 8, gfx_settings.width, //
      5, 0x2B, 0, 0, gfx_settings.height >> 8, gfx_settings.height, //
#endif
      1, 0x20,                  // no invert colours
      2, 0x3A, 0x55,            // 16bpp
      3, 0xB1, 0x00, ILI9341_UPDATE_FRAMERATE,  //
      4, 0xB6, 0x08, 0x82, 0x27,        //
      2, 0xF2, 0x02,            //
      2, 0x26, 0x01,            //
      15, 0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09,     // Gamma +
      15, 0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36,     // Gamma -
      1, 0x11,                  // Sleep out
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
                 + (gfx_settings.flip & 1 ? 0 : 0x40)
#endif
      );                        // bit3:RGB, bit5:rowcolswap, bit6:colrev, bit7:rowrev
   usleep (120000);
   gfx_driver_send ();
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
