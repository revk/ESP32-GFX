const unsigned char * const gfx_7seg_pack1[]={// 7x9 (1 bpp), 9 bytes per character
// packing bytes: 3 bits lx + 5 bits nx-1, 8 bits ly, 1 bit lx + 7 bits ny-1
(const unsigned char[]){0x00,0x00,0x00,	// a  
0x70},
(const unsigned char[]){0x00,0x01,0x02,	// b  
0x08,
0x08,
0x08},
(const unsigned char[]){0x00,0x05,0x02,	// c  
0x08,
0x08,
0x08},
(const unsigned char[]){0x00,0x08,0x00,	// d  
0x70},
(const unsigned char[]){0x00,0x05,0x02,	// e  
0x80,
0x80,
0x80},
(const unsigned char[]){0x00,0x01,0x02,	// f  
0x80,
0x80,
0x80},
(const unsigned char[]){0x00,0x04,0x00,	// g  
0x70},
NULL,				// h  
NULL,				// i  
NULL,				// j  
};
