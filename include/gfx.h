// Simple display and text logic
// Copyright © 2019-22 Adrian Kennard Andrews & Arnold Ltd

typedef	uint8_t gfx_intensity_t; // The intensity of a pixel 0-255
typedef int16_t gfx_pos_t;	 // X/Y location of a pixel - off screen allowed (and ignored)
typedef	uint8_t gfx_align_t;	// Alignment options (GFX_x)

#define	GFX_T	0x01	// top align
#define	GFX_M	0x03	// middle align
#define	GFX_B	0x02	// bottom align
#define	GFX_V	0x08	// vertical move
#define	GFX_L	0x10	// left align
#define	GFX_C	0x30	// centre align
#define	GFX_R	0x20	// right align
#define	GFX_H	0x80	// horizontal move

// Set up SPI, and start the update task (note GPIO 0 not used)
typedef struct {
 uint8_t port;	// The SPI port to use, if -ve then se default
 uint8_t cs;	// The GPIO port for CS (Chip select), if 0 then default / not used
 uint8_t dc;	// The GPIO port for DS (Data/Commant select), if 0 then default 
 uint8_t sck;	// The GPIO port for SKC (Clock), if 0 then default
 uint8_t mosi;	// The GPIO port for MOSI (Master out / slave in), if 0 then default
 uint8_t miso;	// The GPIO port for MISO (Master in / slave out), if 0 then default / not used
 uint8_t rst;	// The GPIO port for RST (Reset), if 0 then default / not used
 uint8_t ena;	// The GPIO port for ENA (Enable), if 0 then default / not used
 uint8_t busy;	// The GPIO port for BUSY (Busy), if 0 then default / not used
 uint8_t contrast;	// Contracts, if display supports it, 0 for default
 uint8_t width;	// Display width
 uint8_t height;	// Display width
 uint8_t flip:3;	// Display flipping
 uint8_t partial:1;	// E-Paper partial updates
 uint8_t mode2:1;	// E-Paper mode 2 updates
 uint8_t sleep:1;	// E-Paper sleep mode
 uint8_t direct:1;	// Update on unlock, not on a task
 // Some dynamic values - don't set in init
 uint8_t changed:1;	// There has been a change, cleared when sent
 uint8_t norefresh:1;	// Next update does not need full refresh, can be set in init to avoid normal startup process (e.g. from deep sleep)
 uint8_t update:1;	// Controls update, cleared when updated
} gfx_init_t;
#define gfx_init(...)  gfx_init_opts((gfx_init_t){__VA_ARGS__})
const char *gfx_init_opts(gfx_init_t);

// locking atomic drawing functions
void gfx_lock(void);	// sets drawing state to 0, 0, left, top, horizontal, white on black
void gfx_unlock(void);	// unlocks display allowing it to be sent if there has been any change
void gfx_refresh(void);	// E-paper mainly, do full mode update
void gfx_wait(void);	// Wait for updates to be done

// Overall display contrast setting if supported by display
void gfx_set_contrast(gfx_intensity_t);

// Drawing functions - do a lock first
// State setting
void gfx_pos(gfx_pos_t x,gfx_pos_t y,gfx_align_t);	// Set position and alignment, note y=0 is TOP of display
void gfx_colour(char);	// Set foreground - colour is a character
void gfx_background(char);	// Set background - colour is a character

// State get
uint8_t	gfx_width(void);	// Display width
uint8_t	gfx_height(void);	// Display height
uint8_t	gfx_bpp(void);	// Display bpp
gfx_pos_t gfx_x(void);	// Current x
gfx_pos_t gfx_y(void); // Current y
gfx_align_t gfx_a(void); // Current alignment
char gfx_f(void);	// Current foreground colour
char gfx_b(void);	// Current background colour

// Drawing
void gfx_pixel(gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i); // set pixel directly (uses current foreogrund/background colour)
void gfx_clear(gfx_intensity_t);	// clear whole display - same as gfx_pixel for all points
void gfx_box(gfx_pos_t w,gfx_pos_t h,gfx_intensity_t); // draw a box, not filled
void gfx_fill(gfx_pos_t w,gfx_pos_t h,gfx_intensity_t); // draw a filled rectangle
void gfx_text(int8_t size, const char *fmt,...); // text, use -ve size for descenders versions
  void
gfx_7seg (int8_t size, const char *fmt, ...); // digits (allows : or , or space after a digit)

void gfx_icon2(gfx_pos_t w,gfx_pos_t h,const void *data);	// Icon, 2 bit per pixel packed
void gfx_icon16(gfx_pos_t w,gfx_pos_t h,const void *data);	// Icon, 16 bit per pixel packed

// General tools
void gfx_message(const char *);	// General full screen message display (lines separated with / and using [colour/size])
