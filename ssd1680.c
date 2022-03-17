// SSD1680 driver code

#define SSD1680_DRIVER_CONTROL 0x01
#define SSD1680_GATE_VOLTAGE 0x03
#define SSD1680_SOURCE_VOLTAGE 0x04
#define SSD1680_PROGOTP_INITIAL 0x08
#define SSD1680_PROGREG_INITIAL 0x09
#define SSD1680_READREG_INITIAL 0x0A
#define SSD1680_BOOST_SOFTSTART 0x0C
#define SSD1680_DEEP_SLEEP 0x10
#define SSD1680_DATA_MODE 0x11
#define SSD1680_SW_RESET 0x12
#define SSD1680_TEMP_CONTROL 0x18
#define SSD1680_TEMP_WRITE 0x1A
#define SSD1680_MASTER_ACTIVATE 0x20
#define SSD1680_DISP_CTRL1 0x21
#define SSD1680_DISP_CTRL2 0x22
#define SSD1680_WRITE_RAM1 0x24
#define SSD1680_WRITE_RAM2 0x26
#define SSD1680_WRITE_VCOM 0x2C
#define SSD1680_READ_OTP 0x2D
#define SSD1680_READ_STATUS 0x2F
#define SSD1680_WRITE_LUT 0x32
#define SSD1680_WRITE_BORDER 0x3C
#define SSD1680_SET_RAMXPOS 0x44
#define SSD1680_SET_RAMYPOS 0x45
#define SSD1680_SET_RAMXCOUNT 0x4E
#define SSD1680_SET_RAMYCOUNT 0x4F

static const char *gfx_driver_init(void)
{                               // Initialise
   const uint8_t ssd1680_default_init_code[] = {
      SSD1680_SW_RESET, 0,      // soft reset
      0xFF, 20,                 // busy wait
      SSD1680_DATA_MODE, 1, 0x03,       // Ram data entry mode
      SSD1680_WRITE_BORDER, 1, 0x05,    // border color

      SSD1680_WRITE_VCOM, 1, 0x36,      // Vcom Voltage
      SSD1680_GATE_VOLTAGE, 1, 0x17,    // Set gate voltage
      SSD1680_SOURCE_VOLTAGE, 3, 0x41, 0x00, 0x32,      // Set source voltage
      SSD1680_SET_RAMXCOUNT, 1, 1,
      SSD1680_SET_RAMYCOUNT, 2, 0, 0,
      SSD1680_SET_RAMXPOS, 2, 1, (CONFIG_GFX_HEIGHT + 7) / 8,
      SSD1680_SET_RAMYPOS, 4, 0, 0, (CONFIG_GFX_WIDTH - 1), (CONFIG_GFX_WIDTH - 1) >> 8,
      SSD1680_DRIVER_CONTROL, 3, (CONFIG_GFX_WIDTH - 1), (CONFIG_GFX_WIDTH - 1) >> 8, 0,
      0xFE
   };

   if (gfx_command_list(ssd1680_default_init_code))
      return "Init failed";
   return NULL;
}

static const char *gfx_driver_send(void)
{                               // Send buffer and update display
   uint8_t buf[2] = { };
   buf[0] = 1;
   if (gfx_command(SSD1680_SET_RAMXCOUNT, buf, 1))
      return "Set X failed";
   buf[0] = 0;
   if (gfx_command(SSD1680_SET_RAMYCOUNT, buf, 2))
      return "Set Y failed";
   if (gfx_command(SSD1680_WRITE_RAM1, NULL, 0))
      return "Write RAM failed";
   if (gfx_send_data(gfx, GFX_SIZE))
      return "Data send failed";
   buf[0] = 0xF7;
   if (gfx_command(SSD1680_DISP_CTRL2, buf, 1))
      return "Display ctrl failed";
   if (gfx_command(SSD1680_MASTER_ACTIVATE, NULL, 0))
      return "Master activate failed";
   gfx_busy_wait();
   return NULL;
}
