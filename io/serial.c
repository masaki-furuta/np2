#include	"compiler.h"
#include	"commng.h"
#include	"pccore.h"
#include	"iocore.h"


static const UINT8 joykeytable[12] = {
				0x2a,	0x34,
				0x29,	0x1c,
				0x3c,	0x48,
				0x3b,	0x46,
				0x3d,	0x4b,
				0x3a,	0x43};

static const UINT8 kbexflag[0x80] = {
		//	 ESC,  １,  ２,  ３,  ４,  ５,  ６,  ７		; 00h
			   0,   0,   0,   0,   0,   0,   0,   0,
		//	  ８,  ９,  ０,  −,  ＾,  ￥,  BS, TAB		; 08h
			   0,   0,   0,   0,   0,   0,   0,   0,
		//	  Ｑ,  Ｗ,  Ｅ,  Ｒ,  Ｔ,  Ｙ,  Ｕ,  Ｉ		; 10h
			   0,   0,   0,   0,   0,   0,   0,   0,
		//	  Ｏ,  Ｐ,  ＠,  ［, Ret,  Ａ,  Ｓ,  Ｄ		; 18h
			   0,   0,   0,   0,   1,   0,   0,   0,
		//	  Ｆ,  Ｇ,  Ｈ,  Ｊ,  Ｋ,  Ｌ,  ；,  ：		; 20h
			   0,   0,   0,   0,   0,   0,   0,   0,
		//    ］,  Ｚ,  Ｘ,  Ｃ,  Ｖ,  Ｂ,  Ｎ,  Ｍ		; 28h
			   0,   1,   1,   0,   0,   0,   0,   0,
		//    ，,  ．,  ／,  ＿, SPC,XFER,RLUP,RLDN		; 30h
			   0,   0,   0,   0,   1,   0,   0,   0,
		//	 INS, DEL,  ↑,  ←,  →,  ↓,HMCR,HELP		; 38h
			   2,   0,   1,   1,   1,   1,   0,   0,
		//	<−>,<／>,<７>,<８>,<９>,<＊>,<４>,<５>		; 40h
			   0,   0,   0,   1,   0,   0,   1,   0,
		//	<６>,<＋>,<１>,<２>,<３>,<＝>,<０>,<，>		; 48h
			   1,   0,   0,   1,   0,   0,   0,   0,
		//	<．>,NFER,vf.1,vf.2,vf.3,vf.4,vf.5,   		; 50h
			   0,   0,   2,   2,   2,   2,   2,   0,
		//	    ,    ,    ,    ,    ,    ,HOME,   		; 58h
			   0,   0,   0,   0,   0,   0,   0,   0,
		//	STOP,COPY, f.1, f.2, f.3, f.4, f.5, f.6		; 60h
			   0,   0,   2,   2,   2,   2,   2,   2,
		//	 f.7, f.8, f.9, f10,    ,    ,    ,   		; 68h
			   2,   2,   2,   2,   0,   0,   0,   0,
		//	 SFT,CAPS,KANA,GRPH,CTRL,    ,    ,   		; 70h
			   2,   2,   2,   2,   2,   0,   0,   0,
		//	    ,    ,    ,    ,    ,    ,    ,   		; 78h
			   0,   0,   0,   0,   0,   0,   0,   0};


static void keybrd_int(BOOL absolute) {

	if (keybrd.buffers) {
		if (!(keybrd.status & 2)) {
			keybrd.status |= 2;
			keybrd.data = keybrd.buf[keybrd.pos];
			keybrd.pos = (keybrd.pos + 1) & KB_BUFMASK;
			keybrd.buffers--;
		}
		pic_setirq(1);
		nevent_set(NEVENT_KEYBOARD, keybrd.xferclock,
											keybrd_callback, absolute);
	}
}

void keybrd_callback(NEVENTITEM item) {

	if (item->flag & NEVENT_SETEVENT) {
		keybrd_int(NEVENT_RELATIVE);
	}
}

static void keybrd_out(REG8 data) {

	if (keybrd.buffers < KB_BUF) {
		keybrd.buf[(keybrd.pos + keybrd.buffers) & KB_BUFMASK] = data;
		keybrd.buffers++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			keybrd_int(NEVENT_ABSOLUTE);
		}
	}
	else {
		keybrd.status |= 0x10;
	}
}


// ----

static	UINT8	keystat[0x80];

void keystat_reset(void) {

	ZeroMemory(keystat, sizeof(keystat));
}


void keystat_senddata(REG8 data) {

	REG8		key;
	BOOL		keynochange;
const _NKEYM	*user;
	UINT		i;

	key = data & 0x7f;
	keynochange = FALSE;

	// CTRL:カナ 0x71,0x72 bit7==0でトグル処理 (標準処理)
	if ((key == 0x71) || (key == 0x72)) {
		if (data & 0x80) {
			return;
		}
		data = key | (keystat[key] & 0x80);
		keystat[key] ^= 0x80;
	}
	else if ((key == 0x76) || (key == 0x77)) {		// user key
		user = np2cfg.userkey + (key - 0x76);
		for (i=0; i<user->keys; i++) {
			key = user->key[i] & 0x7f;
			if (!((keystat[key] ^ data) & 0x80)) {
				keystat[key] ^= 0x80;
				keybrd_out((REG8)(key | (data & 0x80)));
			}
		}
		return;
	}
	else {
		if ((np2cfg.XSHIFT) &&
			(((key == 0x70) && (np2cfg.XSHIFT & 1)) ||
			((key == 0x74) && (np2cfg.XSHIFT & 2)) ||
			((key == 0x73) && (np2cfg.XSHIFT & 4)))) {
			if (data & 0x80) {
				return;
			}
			data = key | (keystat[key] & 0x80);
			keystat[key] ^= 0x80;
		}
		else {
			// CTRL:カナ 0x79,0x7a bit7をそのまま通知
			// (ハードウェアでメカニカル処理してる場合)
			if ((key == 0x79) || (key == 0x7a)) {
				key -= 0x08;
				data -= 0x08;
			}
			if (!((keystat[key] ^ data) & 0x80)) {
				keystat[key] ^= 0x80;
			}
			else {
				keynochange = TRUE;
				if (kbexflag[key] & 2) {			// キーリピート無し
					return;
				}
			}
		}
	}
	if ((!np2cfg.KEY_MODE) || (!(kbexflag[key] & 1))) {
		if (keynochange) {
			if (data & 0x80) {						// ver0.30
				return;
			}
			keybrd_out((REG8)(data ^ 0x80));
		}
		keybrd_out(data);
	}
}

void keystat_forcerelease(REG8 value) {

	REG8		key;
const _NKEYM	*user;
	UINT		i;

	key = value & 0x7f;
	if ((key != 0x76) && (key != 0x77)) {
		if (keystat[key] & 0x80) {
			keystat[key] &= ~0x80;
			keybrd_out((REG8)(key | 0x80));
		}
	}
	else {
		user = np2cfg.userkey + (key - 0x76);
		for (i=0; i<user->keys; i++) {
			key = user->key[i] & 0x7f;
			if (keystat[key] & 0x80) {
				keystat[key] &= ~0x80;
				keybrd_out((REG8)(key | 0x80));
			}
		}
	}
}

void keystat_resetcopyhelp(void) {

	REG8	i;

	for (i=0x60; i<0x62; i++) {
		if (keystat[i] & 0x80) {
			keystat[i] &= 0x7f;
			keybrd_out((REG8)(i | 0x80));
		}
	}
}

void keystat_allrelease(void) {

	REG8	i;

	for (i=0; i<0x80; i++) {
		if (keystat[i] & 0x80) {
			keystat[i] &= ~0x80;
			keybrd_out((REG8)(i | 0x80));
		}
	}
}

void keystat_resetjoykey(void) {

	int		i;
	REG8	key;

	for (i=0; i<12; i++) {
		key = joykeytable[i];
		if (keystat[key] & 0x80) {
			keystat[key] &= 0x7f;
			keybrd_out((REG8)(key | 0x80));
		}
	}
}


// ----

typedef struct {
	UINT8	joysync;
	UINT8	joylast;
	UINT8	mouselast;
	UINT8	padding;
	UINT8	d_up;
	UINT8	d_dn;
	UINT8	d_lt;
	UINT8	d_rt;
} KEYEXT;

static	KEYEXT	keyext;
static const UINT8 mousedelta[] = {1, 1, 1, 1,
									2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 4};
#define	MOUSESTEPMAX ((sizeof(mousedelta) / sizeof(UINT8)) - 1)

void keyext_flash(void) {

	keyext.joysync = 0;
}

BYTE keyext_getjoy(void) {

	BYTE	flg;
const BYTE	*p;
	BYTE	bit;

	if (!keyext.joysync) {
		keyext.joysync = 1;
		flg = 0xff;
		p = joykeytable;
		for (bit=0x20; bit; bit>>=1) {
			if ((keystat[p[0]] & 0x80) || (keystat[p[1]] & 0x80)) {
				flg ^= bit;
			}
			p += 2;
		}
		keyext.joylast = flg;
	}
	return(keyext.joylast);
}

BYTE keyext_getmouse(SINT16 *x, SINT16 *y) {

	BYTE	btn;
	BYTE	acc;
	SINT16	tmp;
	BYTE	ret;

	btn = keyext_getjoy();
	acc = btn | keyext.mouselast;
	keyext.mouselast = btn;
	tmp = 0;
	if (!(btn & 1)) {
		tmp -= mousedelta[keyext.d_up];
	}
	if (!(acc & 1)) {
		if (keyext.d_up < MOUSESTEPMAX) {
			keyext.d_up++;
		}
	}
	else {
		keyext.d_up = 0;
	}
	if (!(btn & 2)) {
		tmp += mousedelta[keyext.d_dn];
	}
	if (!(acc & 2)) {
		if (keyext.d_dn < MOUSESTEPMAX) {
			keyext.d_dn++;
		}
	}
	else {
		keyext.d_dn = 0;
	}
	*y += tmp;

	tmp = 0;
	if (!(btn & 4)) {
		tmp -= mousedelta[keyext.d_lt];
	}
	if (!(acc & 4)) {
		if (keyext.d_lt < MOUSESTEPMAX) {
			keyext.d_lt++;
		}
	}
	else {
		keyext.d_lt = 0;
	}
	if (!(btn & 8)) {
		tmp += mousedelta[keyext.d_rt];
	}
	if (!(acc & 8)) {
		if (keyext.d_rt < MOUSESTEPMAX) {
			keyext.d_rt++;
		}
	}
	else {
		keyext.d_rt = 0;
	}
	*x += tmp;

	ret = 0x5f;
	ret += (btn & 0x10) << 3;
	ret += (btn & 0x20);
	return(ret);
}


// ----

static void IOOUTCALL keybrd_o41(UINT port, REG8 dat) {

	keybrd.mode = dat;
	(void)port;
}

static void IOOUTCALL keybrd_o43(UINT port, REG8 dat) {

	if ((!(dat & 0x08)) && (keybrd.cmd & 0x08)) {
		keyboard_resetsignal();
	}
	if (dat & 0x10) {
		keybrd.status &= ~(0x38);
	}
	keybrd.cmd = dat;
	(void)port;
}

static REG8 IOINPCALL keybrd_i41(UINT port) {

	(void)port;
	keybrd.status &= ~2;
	return(keybrd.data);
}

static REG8 IOINPCALL keybrd_i43(UINT port) {

	(void)port;
	return(keybrd.status);
}


// ----

static const IOOUT keybrdo41[2] = {
					keybrd_o41,	keybrd_o43};

static const IOINP keybrdi41[2] = {
					keybrd_i41,	keybrd_i43};


void keyboard_reset(void) {

	ZeroMemory(&keybrd, sizeof(keybrd));
	keybrd.data = 0xff;
	keybrd.mode = 0x5e;
}

void keyboard_bind(void) {

	keybrd.xferclock = pccore.realclock / 1920;
	iocore_attachsysoutex(0x0041, 0x0cf1, keybrdo41, 2);
	iocore_attachsysinpex(0x0041, 0x0cf1, keybrdi41, 2);
}

void keyboard_resetsignal(void) {									// ver0.29

	int		i;

	keybrd.mode = 0x5e;
	keybrd.cmd = 0;
	keybrd.status = 0;
	keybrd.buffers = 0;
	keybrd.pos = 0;
	for (i=0; i<0x80; i++) {
		if (keystat[i]) {
			keybrd_out((REG8)i);
		}
	}
}


// -----------------------------------------------------------------------


	COMMNG	cm_rs232c;

void rs232c_construct(void) {

	cm_rs232c = NULL;
}

void rs232c_destruct(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
}

void rs232c_open(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
	}
}

void rs232c_callback(void) {

	BOOL	interrupt;

	interrupt = FALSE;
	if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c.data))) {
		rs232c.result |= 2;
		if (sysport.c & 1) {
			interrupt = TRUE;
		}
	}
	else {
		rs232c.result &= (BYTE)~2;
	}
	if (sysport.c & 4) {
		if (rs232c.send) {
			rs232c.send = 0;
			interrupt = TRUE;
		}
	}
	if (interrupt) {
		pic_setirq(4);
	}
}

BYTE rs232c_stat(void) {

	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
	}
	return(cm_rs232c->getstat(cm_rs232c));
}

void rs232c_midipanic(void) {

	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_MIDIRESET, 0);
	}
}


// ----

static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {

	if (cm_rs232c) {
		cm_rs232c->write(cm_rs232c, (UINT8)dat);
	}
	if (sysport.c & 4) {
		rs232c.send = 0;
		pic_setirq(4);
	}
	else {
		rs232c.send = 1;
	}
	(void)port;
}

static void IOOUTCALL rs232c_o32(UINT port, REG8 dat) {

	if (!(dat & 0xfd)) {
		rs232c.dummyinst++;
	}
	else {
		if ((rs232c.dummyinst >= 3) && (dat == 0x40)) {
			rs232c.pos = 0;
		}
		rs232c.dummyinst = 0;
	}
	switch(rs232c.pos) {
		case 0x00:			// reset
			rs232c.pos++;
			break;

		case 0x01:			// mode
			if (!(dat & 0x03)) {
				rs232c.mul = 10 * 16;
			}
			else {
				rs232c.mul = ((dat >> 1) & 6) + 10;
				if (dat & 0x10) {
					rs232c.mul += 2;
				}
				switch(dat & 0xc0) {
					case 0x80:
						rs232c.mul += 3;
						break;
					case 0xc0:
						rs232c.mul += 4;
						break;
					default:
						rs232c.mul += 2;
						break;
				}
				switch(dat & 0x03) {
					case 0x01:
						rs232c.mul >>= 1;
						break;
					case 0x03:
						rs232c.mul *= 32;
						break;
					default:
						rs232c.mul *= 8;
						break;
				}
			}
			rs232c.pos++;
			break;

		case 0x02:			// cmd
			rs232c.pos++;
			break;
	}
	(void)port;
}

static REG8 IOINPCALL rs232c_i30(UINT port) {

	(void)port;
	return(rs232c.data);
}

static REG8 IOINPCALL rs232c_i32(UINT port) {

	if (!(rs232c_stat() & 0x20)) {
		return(rs232c.result | 0x80);
	}
	else {
		(void)port;
		return(rs232c.result);
	}
}


// ----

static const IOOUT rs232co30[2] = {
					rs232c_o30,	rs232c_o32};

static const IOINP rs232ci30[2] = {
					rs232c_i30,	rs232c_i32};

void rs232c_reset(void) {

	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
	rs232c.result = 0x05;
	rs232c.data = 0xff;
	rs232c.send = 1;
	rs232c.pos = 0;
	rs232c.dummyinst = 0;
	rs232c.mul = 10 * 16;
}

void rs232c_bind(void) {

	iocore_attachsysoutex(0x0030, 0x0cf1, rs232co30, 2);
	iocore_attachsysinpex(0x0030, 0x0cf1, rs232ci30, 2);
}

