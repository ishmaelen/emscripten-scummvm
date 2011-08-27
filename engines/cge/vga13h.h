/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This code is based on original Soltys source code
 * Copyright (c) 1994-1995 Janus B. Wisniewski and L.K. Avalon
 */

#ifndef __CGE_VGA13H__
#define __CGE_VGA13H__

#include "common/serializer.h"
#include "graphics/surface.h"
#include "cge/general.h"
#include "cge/bitmap.h"
#include "cge/snail.h"
#include "cge/cge.h"

namespace CGE {

#define kFadeStep        2
#define kVgaColDark      207
#define kVgaColDarkGray  225 /*219*/
#define kVgaColGray      231
#define kVgaColLightGray 237
#define kPixelTransp     0xFE
#define kNoSeq           (-1)
#define kNoPtr           ((uint8)-1)
#define kSprExt          ".SPR"
#define kPalCount        256
#define kPalSize         (kPalCount * 3)


struct Seq {
	uint8 _now;
	uint8 _next;
	int8 _dx;
	int8 _dy;
	int _dly;
};

extern Seq _seq1[];
extern Seq _seq2[];

class SprExt {
public:
	int _x0;
	int _y0;
	int _x1;
	int _y1;
	BitmapPtr _b0;
	BitmapPtr _b1;
	BitmapPtr *_shpList;
	Seq *_seq;
	char *_name;
	Snail::Com *_near;
	Snail::Com *_take;
	SprExt() :
		_x0(0), _y0(0),
		_x1(0), _y1(0),
		_b0(NULL), _b1(NULL),
		_shpList(NULL), _seq(NULL),
		_name(NULL), _near(NULL), _take(NULL)
	{}
};

class Sprite {
protected:
	SprExt *_ext;
public:
	int _ref;
	signed char _cave;
	struct Flags {
		uint16 _hide : 1;       // general visibility switch
		uint16 _near : 1;       // Near action lock
		uint16 _drag : 1;       // sprite is moveable
		uint16 _hold : 1;       // sprite is held with mouse
		uint16 _____ : 1;       // intrrupt driven animation
		uint16 _slav : 1;       // slave object
		uint16 _syst : 1;       // system object
		uint16 _kill : 1;       // dispose memory after remove
		uint16 _xlat : 1;       // 2nd way display: xlat table
		uint16 _port : 1;       // portable
		uint16 _kept : 1;       // kept in pocket
		uint16 _east : 1;       // talk to east (in opposite to west)
		uint16 _shad : 1;       // shadow
		uint16 _back : 1;       // 'send to background' request
		uint16 _bDel : 1;       // delete bitmaps in ~SPRITE
		uint16 _tran : 1;       // transparent (untouchable)
	} _flags;
	int _x;
	int _y;
	signed char _z;
	uint16 _w;
	uint16 _h;
	uint16 _time;
	uint8 _nearPtr;
	uint8 _takePtr;
	int _seqPtr;
	int _shpCnt;
	char _file[kMaxFile];
	Sprite *_prev;
	Sprite *_next;

	bool works(Sprite *spr);
	bool seqTest(int n);
	inline bool active() {
		return _ext != NULL;
	}

	Sprite(CGEEngine *vm, BitmapPtr *shp);
	virtual ~Sprite();
	BitmapPtr shp();
	BitmapPtr *setShapeList(BitmapPtr *shp);
	void moveShapes(uint8 *buf);
	Sprite *expand();
	Sprite *contract();
	Sprite *backShow(bool fast = false);
	void setName(char *newName);
	inline char *name() {
		return (_ext) ? _ext->_name : NULL;
	}
	void gotoxy(int x, int y);
	void center();
	void show();
	void hide();
	BitmapPtr ghost();
	void show(uint16 pg);
	void makeXlat(uint8 *x);
	void killXlat();
	void step(int nr = -1);
	Seq *setSeq(Seq *seq);
	Snail::Com *snList(SnList type);
	virtual void touch(uint16 mask, int x, int y);
	virtual void tick();
	void sync(Common::Serializer &s);
private:
	CGEEngine *_vm;
};

class Queue {
	Sprite *_head;
	Sprite *_tail;
public:
	Queue(bool show);
	~Queue();

	bool _show;

	void append(Sprite *spr);
	void insert(Sprite *spr, Sprite *nxt);
	void insert(Sprite *spr);
	Sprite *remove(Sprite *spr);
	void forAll(void (*fun)(Sprite *));
	Sprite *first() {
		return _head;
	}
	Sprite *last() {
		return _tail;
	}
	Sprite *locate(int ref);
	void clear();
};

class Vga {
	bool _setPal;
	Dac *_oldColors;
	Dac *_newColors;
	const char *_msg;
	const char *_name;

	void updateColors();
	void setColors();
	void waitVR();
public:
	uint32 _frmCnt;
	Queue *_showQ;
	Queue *_spareQ;
	int _mono;
	Graphics::Surface *_page[4];
	Dac *_sysPal;

	Vga();
	~Vga();

	void getColors(Dac *tab);
	void setColors(Dac *tab, int lum);
	void clear(uint8 color);
	void copyPage(uint16 d, uint16 s);
	void sunrise(Dac *tab);
	void sunset();
	void show();
	void update();

	static void palToDac(const byte *palData, Dac *tab);
	static void dacToPal(const Dac *tab, byte *palData);
};

class HorizLine: public Sprite {
public:
	HorizLine(CGEEngine *vm);
};

class CavLight: public Sprite {
public:
	CavLight(CGEEngine *vm);
};

class Spike: public Sprite {
public:
	Spike(CGEEngine *vm);
};

class PocLight: public Sprite {
public:
	PocLight(CGEEngine *vm);
};

Dac mkDac(uint8 r, uint8 g, uint8 b);

template <class CBLK>
uint8 closest(CBLK *pal, CBLK x) {
#define f(col, lum) ((((uint16)(col)) << 8) / lum)
	uint16 i, dif = 0xFFFF, found = 0;
	uint16 L = x._r + x._g + x._b;
	if (!L)
		L++;
	uint16 R = f(x._r, L), G = f(x._g, L), B = f(x._b, L);
	for (i = 0; i < 256; i++) {
		uint16 l = pal[i]._r + pal[i]._g + pal[i]._b;
		if (!l)
			l++;
		int  r = f(pal[i]._r, l), g = f(pal[i]._g, l), b = f(pal[i]._b, l);
		uint16 D = ((r > R) ? (r - R) : (R - r)) +
		           ((g > G) ? (g - G) : (G - g)) +
		           ((b > B) ? (b - B) : (B - b)) +
		           ((l > L) ? (l - L) : (L - l)) * 10 ;

		if (D < dif) {
			found = i;
			dif = D;
			if (D == 0)
				break;    // exact!
		}
	}
	return found;
#undef f
}

Sprite *spriteAt(int x, int y);
Sprite *locate(int ref);

} // End of namespace CGE

#endif
