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

static const char *gfx_driver_init(void)
{                               // Initialise
   const uint8_t ssd1681_default_init_code[] = {
      SSD1681_SW_RESET, 0,      // soft reset
      0xFF, 20,                 // busy wait
      SSD1681_DRIVER_CONTROL, 3, (gfx_width() - 1), (gfx_width() - 1) >> 8, 0,
      SSD1681_DATA_MODE, 1, 0x03,       // Ram data entry mode
      SSD1681_WRITE_BORDER, 1, 0x05,    // border color
      SSD1681_TEMP_CONTROL, 1, 0x80,    // Temp control
      SSD1681_SET_RAMXCOUNT, 1, 0,
      SSD1681_SET_RAMYCOUNT, 2, 0, 0,
      0xFE
   };

   if (gfx_command_list(ssd1681_default_init_code))
      return "Init failed";
   return NULL;
}

static const char *gfx_driver_send(void)
{                               // Send buffer and update display
   uint8_t buf[2] = {0};
   if (gfx_command(SSD1681_SET_RAMXCOUNT, buf, 1))
      return "Set X failed";
   if (gfx_command(SSD1681_SET_RAMYCOUNT, buf, 2))
      return "Set Y failed";
   if (gfx_command(SSD1681_WRITE_RAM1, NULL, 0))
      return "Write RAM failed";
   if (gfx_send_gfx())
      return "Data send failed";
   buf[0] = 0xF7; // DISPLAY with DISPLAY Mode 1
   if (gfx_command(SSD1681_DISP_CTRL2, buf, 1))
      return "Display ctrl failed";
   if (gfx_command(SSD1681_MASTER_ACTIVATE, NULL, 0))
      return "Master activate failed";
   gfx_busy_wait();
   return NULL;
}
