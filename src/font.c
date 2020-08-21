/**
 * Font rendering library for SDL2.
 * Copyright (C) 2020  Mahyar Koshkouei
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <SDL.h>

#include <font.h>

struct font_ctx_s {
	SDL_Texture *tex;
	SDL_Renderer *rend;
};

#define FONT_BITMAP_WIDTH 144
#define FONT_BITMAP_HEIGHT 90
#define FONT_BITMAP_SIZE ((FONT_BITMAP_HEIGHT * FONT_BITMAP_WIDTH) / 8)
#define FONT_COLUMNS 16
#define FONT_PITCH (FONT_BITMAP_WIDTH / 8)

static const unsigned char bitmap_font[FONT_BITMAP_SIZE] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x18, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0d, 0x80, 0x00, 0x82,
	0x18, 0xe0, 0x30, 0x0c, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
	0x00, 0x0c, 0x0d, 0x86, 0xc3, 0xe7, 0x31, 0xb0, 0x30, 0x18, 0x06, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c, 0x0d, 0x86, 0xc6, 0xb7,
	0x31, 0xb0, 0x30, 0x18, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0c,
	0x00, 0x0c, 0x00, 0x0f, 0xe6, 0x82, 0x61, 0xe0, 0x30, 0x30, 0x03, 0x0d,
	0x83, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x0c, 0x00, 0x06, 0xc7, 0x80,
	0xc0, 0xc0, 0x00, 0x30, 0x03, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x30,
	0x00, 0x0c, 0x00, 0x06, 0xc3, 0xe0, 0xc1, 0xe4, 0x00, 0x30, 0x03, 0x1f,
	0xdf, 0xe0, 0x07, 0xf8, 0x00, 0x30, 0x00, 0x0c, 0x00, 0x0f, 0xe0, 0xf1,
	0x93, 0x3c, 0x00, 0x30, 0x03, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x60,
	0x00, 0x0c, 0x00, 0x06, 0xc0, 0xb3, 0x3b, 0x18, 0x00, 0x18, 0x06, 0x0d,
	0x83, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0xc6, 0xb3,
	0x3b, 0x38, 0x00, 0x18, 0x06, 0x00, 0x03, 0x01, 0xc0, 0x00, 0x60, 0xc0,
	0x00, 0x0c, 0x00, 0x00, 0x03, 0xe6, 0x11, 0xec, 0x00, 0x0c, 0x0c, 0x00,
	0x00, 0x01, 0xc0, 0x00, 0xf1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x00, 0x00, 0x00, 0x06, 0x18, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x18, 0x0c, 0x0f, 0x0f, 0x80, 0x67, 0xf0, 0xf1, 0xfe, 0x3c, 0x1e, 0x00,
	0x00, 0x00, 0x60, 0x01, 0x80, 0x7c, 0x3c, 0x1c, 0x19, 0x98, 0xc0, 0xe6,
	0x01, 0x98, 0x06, 0x66, 0x33, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xc0, 0xc6,
	0x66, 0x3c, 0x30, 0xc0, 0x61, 0xe6, 0x03, 0x08, 0x06, 0xc3, 0x61, 0x80,
	0x00, 0x01, 0x80, 0x00, 0x60, 0xc6, 0xc3, 0x0c, 0x00, 0xc0, 0xc3, 0x66,
	0xe3, 0x00, 0x0c, 0x66, 0x61, 0x86, 0x03, 0x03, 0x00, 0x00, 0x30, 0x06,
	0xdb, 0x0c, 0x01, 0x83, 0x86, 0x67, 0x33, 0x70, 0x18, 0x3c, 0x33, 0x8f,
	0x07, 0x86, 0x07, 0xf8, 0x18, 0x0c, 0xdb, 0x0c, 0x03, 0x00, 0xcc, 0x60,
	0x1b, 0x98, 0x18, 0x66, 0x1d, 0x86, 0x03, 0x06, 0x00, 0x00, 0x18, 0x18,
	0xc3, 0x0c, 0x06, 0x00, 0x6f, 0xf0, 0x1b, 0x0c, 0x30, 0xc3, 0x01, 0x80,
	0x00, 0x03, 0x00, 0x00, 0x30, 0x30, 0x66, 0x0c, 0x0c, 0x00, 0x60, 0x66,
	0x1b, 0x0c, 0x30, 0xc3, 0x21, 0x80, 0x00, 0x01, 0x87, 0xf8, 0x60, 0x00,
	0x3c, 0x0c, 0x18, 0x18, 0xc0, 0x63, 0x31, 0x98, 0x30, 0x66, 0x33, 0x06,
	0x03, 0x80, 0xc0, 0x00, 0xc0, 0x30, 0x18, 0x3f, 0x3f, 0xcf, 0x80, 0x61,
	0xe0, 0xf0, 0x30, 0x3c, 0x1e, 0x0f, 0x03, 0x80, 0x60, 0x01, 0x80, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
	0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x0c, 0x3f, 0x07, 0xcf, 0xc3,
	0xfb, 0xfc, 0x7c, 0xc3, 0x3f, 0x07, 0x98, 0x66, 0x06, 0x1b, 0x0c, 0x78,
	0x66, 0x1e, 0x31, 0x8c, 0x6c, 0x63, 0x03, 0x00, 0xc6, 0xc3, 0x0c, 0x01,
	0x98, 0xc6, 0x07, 0x3b, 0x8c, 0xcc, 0xc3, 0x33, 0x30, 0xd8, 0x2c, 0x33,
	0x03, 0x01, 0x80, 0xc3, 0x0c, 0x01, 0x99, 0x86, 0x07, 0xfb, 0xcd, 0x86,
	0xcf, 0x61, 0xb1, 0x98, 0x0c, 0x33, 0x03, 0x01, 0x80, 0xc3, 0x0c, 0x01,
	0x9b, 0x06, 0x06, 0xdb, 0xcd, 0x86, 0xdb, 0x61, 0xbf, 0x18, 0x0c, 0x33,
	0xf3, 0xf1, 0x80, 0xff, 0x0c, 0x01, 0x9e, 0x06, 0x06, 0xdb, 0x6d, 0x86,
	0xdb, 0x61, 0xb1, 0x98, 0x0c, 0x33, 0x03, 0x01, 0x8e, 0xc3, 0x0c, 0x01,
	0x9e, 0x06, 0x06, 0xdb, 0x6d, 0x86, 0xce, 0x7f, 0xb0, 0xd8, 0x0c, 0x33,
	0x03, 0x01, 0x86, 0xc3, 0x0c, 0x01, 0x9b, 0x06, 0x06, 0x1b, 0x3d, 0x86,
	0xc0, 0x61, 0xb0, 0xd8, 0x2c, 0x33, 0x03, 0x01, 0x86, 0xc3, 0x0c, 0x11,
	0x99, 0x86, 0x06, 0x1b, 0x1d, 0x86, 0x63, 0x61, 0xb1, 0x8c, 0x6c, 0x63,
	0x03, 0x00, 0xc6, 0xc3, 0x0c, 0x1b, 0x18, 0xc6, 0x06, 0x1b, 0x1c, 0xcc,
	0x3e, 0x61, 0xbf, 0x07, 0xcf, 0xc3, 0xfb, 0x00, 0x7c, 0xc3, 0x3f, 0x0e,
	0x18, 0x67, 0xf6, 0x1b, 0x0c, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x01, 0xf0, 0x00, 0x00,
	0xfe, 0x1e, 0x3f, 0x8f, 0xcf, 0xf6, 0x1b, 0x0d, 0x86, 0xc3, 0x61, 0x9f,
	0xc6, 0x0c, 0x00, 0x30, 0x60, 0x00, 0xc3, 0x33, 0x30, 0xd8, 0x61, 0x86,
	0x1b, 0x0d, 0x86, 0xc3, 0x61, 0x80, 0xc6, 0x06, 0x00, 0x30, 0xf0, 0x00,
	0xc3, 0x61, 0xb0, 0xd8, 0x01, 0x86, 0x1b, 0x0d, 0x86, 0x66, 0x33, 0x00,
	0xc6, 0x06, 0x00, 0x31, 0x98, 0x00, 0xc3, 0x61, 0xb0, 0xd8, 0x01, 0x86,
	0x19, 0x99, 0x86, 0x3c, 0x1e, 0x01, 0x86, 0x03, 0x00, 0x31, 0x08, 0x00,
	0xfe, 0x61, 0xbf, 0x8f, 0xc1, 0x86, 0x19, 0x99, 0xb6, 0x18, 0x0c, 0x03,
	0x06, 0x01, 0x80, 0x30, 0x00, 0x00, 0xc0, 0x61, 0xbe, 0x00, 0x61, 0x86,
	0x19, 0x99, 0xb6, 0x18, 0x0c, 0x06, 0x06, 0x01, 0x80, 0x30, 0x00, 0x00,
	0xc0, 0x6d, 0xb3, 0x00, 0x61, 0x86, 0x18, 0xf1, 0xb6, 0x3c, 0x0c, 0x0c,
	0x06, 0x00, 0xc0, 0x30, 0x00, 0x00, 0xc0, 0x67, 0xb1, 0x80, 0x61, 0x86,
	0x18, 0xf1, 0xfe, 0x66, 0x0c, 0x18, 0x06, 0x00, 0x60, 0x30, 0x00, 0x00,
	0xc0, 0x33, 0x30, 0xd8, 0x61, 0x83, 0x30, 0x61, 0xce, 0xc3, 0x0c, 0x18,
	0x06, 0x00, 0x60, 0x30, 0x00, 0x00, 0xc0, 0x1e, 0xb0, 0xcf, 0xc1, 0x81,
	0xe0, 0x61, 0x86, 0xc3, 0x0c, 0x1f, 0xc6, 0x00, 0x30, 0x30, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0xc0, 0x01, 0xf0, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x30, 0x00, 0x00, 0x30,
	0x00, 0x78, 0x00, 0xc0, 0x0c, 0x00, 0xcc, 0x03, 0x80, 0x00, 0x00, 0x00,
	0x0c, 0x00, 0x30, 0x00, 0x00, 0x30, 0x00, 0xcc, 0x00, 0xc0, 0x0c, 0x00,
	0xcc, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x30,
	0x00, 0xcc, 0x00, 0xc0, 0x00, 0x00, 0x0c, 0x01, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x1f, 0x37, 0x07, 0xc3, 0xb1, 0xe0, 0xc0, 0xfa, 0xdc, 0x1c, 0x03,
	0xcc, 0xc1, 0x85, 0xb3, 0x70, 0x78, 0x00, 0x31, 0xb9, 0x8c, 0x66, 0x73,
	0x30, 0xc1, 0x8e, 0xe6, 0x0c, 0x00, 0xcd, 0x81, 0x86, 0xdb, 0x98, 0xcc,
	0x00, 0x01, 0xb0, 0xd8, 0x0c, 0x36, 0x1b, 0xf1, 0x8c, 0xc3, 0x0c, 0x00,
	0xcf, 0x01, 0x86, 0xdb, 0x0d, 0x86, 0x00, 0x3f, 0xb0, 0xd8, 0x0c, 0x37,
	0xf8, 0xc1, 0x8c, 0xc3, 0x0c, 0x00, 0xcf, 0x01, 0x86, 0xdb, 0x0d, 0x86,
	0x00, 0x61, 0xb0, 0xd8, 0x0c, 0x36, 0x00, 0xc0, 0xf8, 0xc3, 0x0c, 0x00,
	0xcd, 0x81, 0x86, 0xdb, 0x0d, 0x86, 0x00, 0x63, 0xb9, 0x8c, 0x66, 0x73,
	0x18, 0xc1, 0x80, 0xc3, 0x0c, 0x00, 0xcc, 0xc1, 0x86, 0xdb, 0x0c, 0xcc,
	0x00, 0x3d, 0xb7, 0x07, 0xc3, 0xb1, 0xf0, 0xc0, 0xfc, 0xc3, 0x3f, 0x18,
	0xcc, 0x67, 0xe6, 0xdb, 0x0c, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x86, 0x00, 0x00, 0x18, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 0x00, 0x18,
	0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xfc, 0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x01, 0x80, 0x61, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x80, 0x63, 0x6c, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x01, 0x80, 0x63, 0x38, 0x00, 0xdc, 0x1d, 0xb7, 0x8f, 0xcf, 0xc6,
	0x1b, 0x0d, 0x86, 0xc3, 0x61, 0x9f, 0x83, 0x01, 0x80, 0x60, 0x00, 0x00,
	0xe6, 0x33, 0x9c, 0xd8, 0x63, 0x06, 0x1b, 0x0d, 0x86, 0x66, 0x61, 0x81,
	0x86, 0x01, 0x80, 0x30, 0x00, 0x00, 0xc3, 0x61, 0x98, 0x18, 0x03, 0x06,
	0x19, 0x99, 0xb6, 0x3c, 0x61, 0x83, 0x06, 0x01, 0x80, 0x30, 0x00, 0x00,
	0xc3, 0x61, 0x98, 0x0f, 0xc3, 0x06, 0x19, 0x99, 0xb6, 0x18, 0x61, 0x86,
	0x03, 0x01, 0x80, 0x60, 0x00, 0x00, 0xc3, 0x61, 0x98, 0x00, 0x63, 0x06,
	0x18, 0xf1, 0xb6, 0x3c, 0x61, 0x8c, 0x03, 0x01, 0x80, 0x60, 0x00, 0x00,
	0xe6, 0x33, 0x98, 0x18, 0x63, 0x33, 0x38, 0xf1, 0xfe, 0x66, 0x33, 0x98,
	0x03, 0x01, 0x80, 0x60, 0x00, 0x00, 0xdc, 0x1d, 0x98, 0x0f, 0xc1, 0xe1,
	0xd8, 0x60, 0xcc, 0xc3, 0x1d, 0x9f, 0x83, 0x01, 0x80, 0x60, 0x00, 0x00,
	0xc0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80,
	0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00, 0xc0, 0x01, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xc0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

font_ctx *FontStartup(SDL_Renderer *renderer)
{
	const SDL_Colour colours[2] = {
		{ 0x00, 0x00, 0x00, 0x00}, /* BG */
		{ 0xFF, 0xFF, 0xFF, 0xFF}  /* FG */
	};
	SDL_Surface *bmp_surf;
	font_ctx *ctx = SDL_malloc(sizeof(font_ctx));
	Uint8 *pixels = SDL_malloc(FONT_BITMAP_SIZE);

	SDL_assert(renderer != NULL);

	if(ctx == NULL || pixels == NULL)
	{
		SDL_SetError("Unable to allocate memory.");
		goto err;
	}

	SDL_memcpy(pixels, bitmap_font, FONT_BITMAP_SIZE);
	bmp_surf = SDL_CreateRGBSurfaceFrom(pixels,
					    FONT_BITMAP_WIDTH,
					    FONT_BITMAP_HEIGHT,
					    1, FONT_PITCH, 0, 0, 0, 0);

	if(bmp_surf == NULL)
		goto err;

	if(SDL_SetPaletteColors(bmp_surf->format->palette, colours, 0, 2) < 0)
	{
		SDL_FreeSurface(bmp_surf);
		goto err;
	}

	if(SDL_SetColorKey(bmp_surf, SDL_TRUE, 0) < 0)
	{
		SDL_FreeSurface(bmp_surf);
		goto err;
	}

	ctx->tex = SDL_CreateTextureFromSurface(renderer, bmp_surf);

	if(ctx->tex == NULL)
	{
		SDL_FreeSurface(bmp_surf);
		goto err;
	}

	ctx->rend = renderer;
	SDL_FreeSurface(bmp_surf);

out:
	SDL_free(pixels);
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

void FontDrawSize(const unsigned text_len, unsigned *w, unsigned *h)
{
	*w = text_len * FONT_CHAR_WIDTH;
	*h = FONT_CHAR_HEIGHT;
}

int FontPrintToRenderer(font_ctx *const ctx, const char *text,
			const SDL_Rect *dstscale)
{
	SDL_Rect font_rect, screen_rect;
	SDL_Rect dst;

	SDL_assert(ctx != NULL);
	SDL_assert(text != NULL);

	if(dstscale == NULL)
	{
		dst.w = 1;
		dst.h = 1;
		dst.x = 0;
		dst.y = 0;
	}
	else
		dst = *dstscale;

	font_rect.w = FONT_CHAR_WIDTH;
	font_rect.h = FONT_CHAR_HEIGHT;

	screen_rect.w = FONT_CHAR_WIDTH * dst.w;
	screen_rect.h = FONT_CHAR_HEIGHT * dst.h;
	screen_rect.x = dst.x;
	screen_rect.y = dst.y;

	{
		Uint8 r, g, b, a;
		SDL_GetRenderDrawColor(ctx->rend, &r, &g, &b, &a);
		SDL_SetTextureColorMod(ctx->tex, r, g, b);
		SDL_SetTextureAlphaMod(ctx->tex, a);
	}

	for(; *text; text++)
	{
		Uint8 pos;
		int ret;

		if(*text >= ' ' && *text <= '~')
			pos = *text - ' ';
		else
			pos = '?' - ' ';

		font_rect.x = (pos % FONT_COLUMNS) * FONT_CHAR_WIDTH;
		font_rect.y = (pos / FONT_COLUMNS) * FONT_CHAR_HEIGHT;

		ret = SDL_RenderCopy(ctx->rend, ctx->tex,
				     &font_rect, &screen_rect);

		if(ret < 0)
			return ret;

		screen_rect.x += screen_rect.w;
	}

	return 0;
}

void FontExit(font_ctx *ctx)
{
	if(ctx == NULL)
		return;

	SDL_DestroyTexture(ctx->tex);
	SDL_free(ctx);
}
