#include	"compiler.h"
#include	"vramhdl.h"
#include	"menubase.h"
#include	"menustr.h"


const char mstr_cfg[] = "Configure";
const char mstr_about[] = "About";



// ----

static const BYTE np2icondat[210] = {		// 32x32
		0x06,0x6f,0x00,0x00,0x3f,0x00,0x2d,0xff,0x00,0x0a,0x0d,0x7f,0x09,
		0x9c,0x0c,0xa3,0xf8,0x0b,0xf8,0x0f,0x94,0x16,0x5a,0x0f,0x94,0x23,
		0xdd,0xc0,0xc0,0xc0,0xc7,0x28,0x5a,0x17,0x0e,0x80,0x80,0x80,0x0b,
		0xc2,0x17,0xe9,0x0b,0x05,0xff,0x18,0xa3,0x17,0x11,0x0d,0x45,0x18,
		0xa6,0x17,0xe6,0x00,0x02,0x85,0x66,0x17,0x2c,0xff,0x14,0x08,0x47,
		0x23,0x68,0xd1,0x17,0x23,0x18,0x8e,0x47,0x1d,0x9c,0x9d,0x03,0x9a,
		0xff,0x00,0x08,0x4a,0x02,0xcb,0x2c,0x17,0xef,0xe3,0x2f,0x17,0xec,
		0x47,0x20,0x17,0xda,0xff,0x70,0x4b,0x03,0x91,0x17,0xff,0x00,0x25,
		0x5f,0x23,0x1c,0x5a,0x03,0x88,0xbb,0x48,0xff,0x13,0x75,0x6e,0xe0,
		0x47,0x3b,0xdd,0xe0,0x17,0x08,0x13,0x7b,0x61,0x6f,0x2f,0x3f,0xff,
		0xdd,0x19,0x31,0x7f,0x8c,0xe2,0x8d,0xb8,0x18,0xa3,0x30,0x3f,0xa4,
		0xd0,0xba,0xbe,0xff,0xf4,0x6c,0x20,0x11,0x1d,0xf2,0x8d,0xa6,0x92,
		0x35,0x77,0xe3,0x2f,0xc8,0x17,0x02,0xff,0x2f,0xef,0xbf,0x23,0x5e,
		0x7f,0x17,0xe2,0xf5,0xc5,0x00,0x05,0x12,0xa0,0x2f,0xfe,0xff,0x35,
		0x1a,0x0e,0xd4,0x02,0x0e,0x05,0xdd,0x4c,0x60,0x17,0xd7,0x14,0xd7,
		0x0c,0x94,0xfc,0x5d,0xa9,0x53,0xc5,0x4d,0xd7,0x5e,0x48,0x7a,0x3f,
		0x00,0x3a};

static const BYTE np2iconmask[95] = {		// 32x32
		0x06,0x5f,0x00,0x00,0x03,0xff,0x00,0x02,0x02,0x04,0x00,0x08,0x04,
		0x4c,0x03,0x03,0xff,0x04,0x0b,0x03,0xce,0x03,0x87,0x04,0x46,0x03,
		0x8e,0x04,0x0c,0x00,0x0e,0x07,0xff,0xff,0x20,0x11,0x07,0x93,0x07,
		0xff,0x07,0xe0,0x08,0x16,0x53,0x4d,0x07,0xff,0x1f,0xb0,0xff,0x10,
		0x1f,0x7b,0x8f,0x17,0xb0,0x10,0x8c,0x17,0x6f,0x9c,0x90,0x06,0x9a,
		0x08,0x50,0xff,0x03,0x05,0x07,0xdb,0x11,0x12,0x17,0x8c,0x1f,0xdf,
		0x07,0xe3,0x06,0x4a,0x50,0x9b,0xf8,0x4f,0x99,0x08,0x20,0x9f,0xff,
		0x2f,0x51,0xef,0x10};

const MENURES np2icon = {32, 32, np2icondat, np2iconmask};

