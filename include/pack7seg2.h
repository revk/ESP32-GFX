const unsigned char * const gfx_7seg_pack2[]={// 14x18 (1 bpp), 36 bytes per character
// packing bytes: 3 bits lx + 5 bits nx-1, 8 bits ly, 1 bit lx + 7 bits ny-1
(const unsigned char[]){0x01,0x00,0x01,	// a  
0x7F,0x80,
0x3F,0x00},
(const unsigned char[]){0x20,0x01,0x07,	// b  
0x40,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0x40},
(const unsigned char[]){0x20,0x09,0x07,	// c  
0x40,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0x40},
(const unsigned char[]){0x01,0x10,0x01,	// d  
0x3F,0x00,
0x7F,0x80},
(const unsigned char[]){0x00,0x09,0x07,	// e  
0x80,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0x80},
(const unsigned char[]){0x00,0x01,0x07,	// f  
0x80,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0xC0,
0x80},
(const unsigned char[]){0x00,0x08,0x01,	// g  
0x7F,
0x7F},
(const unsigned char[]){0x20,0x0C,0x01,	// h  
0x18,
0x18},
(const unsigned char[]){0x20,0x04,0x01,	// i  
0x18,
0x18},
(const unsigned char[]){0x20,0x10,0x01,	// j  
0x18,
0x18},
};
