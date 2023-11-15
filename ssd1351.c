// SSD1351 driver code

#define	GFX_DEFAULT_WIDTH	128
#define	GFX_DEFAULT_HEIGHT	128
#define	GFX_BPP			16

static const char *
gfx_driver_init (void)
{                               // Initialise
   int try = 10;
   esp_err_t e = 0;
   usleep (300000);             // 300ms to start up
   while (try--)
   {
      gfx_lock ();
      e = gfx_send_command (0xAF);      // start
      usleep (10000);
      // Many of these are setting as defaults, just to be sure
      e += gfx_send_command (0xA5);     // white
      e += gfx_command1 (0xA0, 0x26);   // colour mode
      e += gfx_command1 (0xFD, 0x12);   // unlock
      e += gfx_command1 (0xFD, 0xB1);   // unlock
      e += gfx_command1 (0xA1, 0x00);   // Start 0
      e += gfx_command1 (0xA2, 0x00);   // Offset 0
#if 0
      e += gfx_command1 (0xB3, 0xF1);   // Frequency
      e += gfx_command1 (0xCA, 0x7F);   // MUX
      e += gfx_command1 (0xAB, 0x01);   // Regulator
      e += gfx_cmd3 (0xB4, 0xA0, 0xB5, 0x55);   // VSL
      e += gfx_cmd3 (0xC1, 0xC8, 0x80, 0xC0);   // Contrast
      e += gfx_command1 (0xC7, 0x0F);   // current
      e += gfx_command1 (0xB1, 0x32);   // clocks
      e += gfx_cmd3 (0xB2, 0xA4, 0x00, 0x00);   // enhance
      e += gfx_command1 (0xBB, 0x17);   // pre-charge voltage
      e += gfx_command1 (0xB6, 0x01);   // pre-charge period
      e += gfx_command1 (0xBE, 0x05);   // COM deselect voltage
#endif
      e += gfx_command1 (0xFD, 0xB0);   // lock
      gfx_command2 (0x15, 0, gfx_settings.width - 1);
      gfx_command2 (0x75, 0, gfx_settings.height - 1);
      //gfx_send_command(0x5C);
      //gfx_send_data(gfx, GFX_SIZE);
      gfx_send_command (0xA6);
      gfx_unlock ();
      if (!e)
         break;
      sleep (1);
   }
   if (e)
      return "Failed to init";
   return NULL;
}

static const char *
gfx_driver_send (void)
{                               // Send buffer and update display
   gfx_command2 (0x15, 0, gfx_settings.width - 1);
   gfx_command2 (0x75, 0, gfx_settings.height - 1);
   gfx_send_command (0x5C);
   gfx_send_gfx ();
   if (gfx_settings.update)
   {
      gfx_settings.update = 0;
      gfx_command1 (0xC7, gfx_settings.contrast >> 4);
   }
   return NULL;
}

static const char *
gfx_driver_sleep (void)
{
   return NULL;
}
