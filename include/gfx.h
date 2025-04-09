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
 uint16_t width;	// Display width
 uint16_t height;	// Display width
 uint8_t flip:3;	// Display flipping
 uint8_t border:2;	// Border black (e-paper)
 uint8_t partial:1;	// E-Paper partial updates
 uint8_t invert:1;	// Invert  (e-paper)
 uint8_t mode2:1;	// E-Paper mode 2 updates
 uint8_t sleep:1;	// E-Paper sleep mode
 uint8_t direct:1;	// Update on unlock, not on a task
 // Some dynamic values - don't set in init
 uint8_t changed:1;	// There has been a change, cleared when sent
 uint8_t norefresh:1;	// Next update does not need full refresh, can be set in init to avoid normal startup process (e.g. from deep sleep)
 uint8_t update:1;	// Controls update, cleared when updated
 uint8_t asleep:1;	// Device is asleep
 uint8_t pause:1;	// Pause needed before next operation
} gfx_init_t;
#define gfx_init(...)  gfx_init_opts((gfx_init_t){__VA_ARGS__})
const char *gfx_init_opts(gfx_init_t);

// locking, etc
void gfx_lock(void);	// sets drawing state to 0, 0, left, top, horizontal, white on black
void gfx_unlock(void);	// unlocks display allowing it to be sent if there has been any change (draws if changed and direct mode)
void gfx_refresh(void);	// E-paper mainly, do full mode update
void gfx_force(void);	// Force update
void gfx_wait(void);	// Wait for updates to be done
void gfx_load(const void *data); // Block load whole image (does not allow for logical invert)
int gfx_ok(void);	// GFX is enabled
void gfx_sleep(void);	// Put device to sleep
void gfx_flip(uint8_t flip);	// Change flip
void gfx_border(uint8_t border);	// Change border

// Overall display contrast setting if supported by display
void gfx_set_contrast(gfx_intensity_t);

// Drawing functions - do a lock first
// State setting
void gfx_pos(gfx_pos_t x,gfx_pos_t y,gfx_align_t);	// Set position and alignment, note y=0 is TOP of display

// For grey/colour displays you can set background and foreground colour.
// For text foreground and background are normally plotted with a 1 pixel border around text (see mask mode below)
// For 7seg, segments are plotted only, as foreground (on) or background (off)
// For epaper (1bpp) you only set K/W (or R for 2bpp e-paper)
// Setting background and foreground the same colour only plots for intensity 255 (using foreground) - i.e. mask mode
void gfx_colour(char);	// Set foreground - colour is a character
void gfx_background(char);	// Set background - colour is a character

// State get
uint16_t	gfx_width(void);	// Display width
uint16_t	gfx_height(void);	// Display height
uint8_t	gfx_bpp(void);	// Display app
gfx_pos_t gfx_x(void);	// Current x
gfx_pos_t gfx_y(void); // Current y
gfx_align_t gfx_a(void); // Current alignment
char gfx_f(void);	// Current foreground colour
char gfx_b(void);	// Current background colour

// Raw
uint16_t gfx_raw_w(void);	// Raw frame buffer width
uint16_t gfx_raw_h(void);	// Raw frame buffer height
void *gfx_raw_b(void);	// Raw frame buffer

// Drawing
void gfx_draw (gfx_pos_t w, gfx_pos_t h, gfx_pos_t wm, gfx_pos_t hm, gfx_pos_t * xp, gfx_pos_t * yp); // Work out drawing position for object
void gfx_pixel(gfx_pos_t x, gfx_pos_t y, gfx_intensity_t i); // set pixel directly (uses current foreogrund/background colour)
void gfx_clear(gfx_intensity_t);	// clear whole display - same as gfx_pixel for all points
void gfx_box(gfx_pos_t w,gfx_pos_t h,gfx_intensity_t); // draw a box, not filled
void gfx_fill(gfx_pos_t w,gfx_pos_t h,gfx_intensity_t); // draw a filled rectangle
void gfx_line(gfx_pos_t x1,gfx_pos_t y1, gfx_pos_t x2, gfx_pos_t y2, gfx_intensity_t); // Draw a line

void gfx_vector(int8_t size, const char *fmt,...); // vector, use -ve size for descenders versions
void gfx_vector_size(int8_t size,const char *,gfx_pos_t *w,gfx_pos_t *h);

// gfx_text flags
#define	GFX_TEXT_DESCENDERS	(1<<0)	// Allow descenders
#define	GFX_TEXT_BLOCKY		(1<<1)	// Blocky text
#define	GFX_TEXT_LIGHT		(1<<2)	// Thin (vector) test
#define	GFX_TEXT_FIXED		(1<<3)	// Don't do narrow special characters (full stop, colon, etc)
#define	GFX_TEXT_ITALIC		(1<<4)	// Italic
void gfx_text(uint8_t flags,uint8_t size, const char *fmt,...); // text, use -ve size for descenders versions
void gfx_text_size(uint8_t flags,uint8_t size,const char *,gfx_pos_t *w,gfx_pos_t *h);
#define	GFX_7SEG_SMALL_DOT	(1<<0)	// Small (half size) after dot
#define	GFX_7SEG_SMALL_COLON	(1<<1)	// Small (half size) after colon
#define	GFX_7SEG_ITALIC		(1<<2)	// Italic
void gfx_7seg (uint8_t flags,int8_t size, const char *fmt, ...); // digits (allows : or , or space after a digit)
void gfx_7seg_size(uint8_t flags,int8_t size,const char *,gfx_pos_t *w,gfx_pos_t *h);

uint8_t gfx_text_desc(const char *c);	// Has decenders

void gfx_icon2(gfx_pos_t w,gfx_pos_t h,const void *data);	// Icon, 2 bit per pixel packed
void gfx_icon16(gfx_pos_t w,gfx_pos_t h,const void *data);	// Icon, 16 bit per pixel packed
const uint8_t * gfx_pack (const uint8_t * data, uint8_t * lx, uint8_t * hx, uint8_t * ly, uint8_t * hy, uint8_t ppb); // Unpacking

// General tools
void gfx_message(const char *);	// General full screen message display (lines separated with / and using [colour/size])
