#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

#include "debug_screen.h"
#include "font.h"

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))

#define GLYPH_SIZE	16
#define BG_COLOR	COLOR_BLACK

extern unsigned char msx_font[];

static SceUID fb_memuid;
static void *fb_base;

static inline void draw_pixel(int x, int y, unsigned int color)
{
	((unsigned int *)fb_base)[x + y * SCREEN_STRIDE] = color;
}

static inline void draw_rectangle(int x, int y, int w, int h, unsigned int color)
{
	int i, j;

	for (i = y; i < y + h; i++)
		for (j = x; j < x + w; j++)
			draw_pixel(j, i, color);
}

int debug_screen_init()
{
	int ret;

	fb_memuid = sceKernelAllocMemBlock("Play!- debug screen",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		ALIGN(SCREEN_STRIDE * SCREEN_HEIGHT * 4, 256*1024), NULL);
	if (fb_memuid < 0)
		return fb_memuid;

	ret = sceKernelGetMemBlockBase(fb_memuid, &fb_base);
	if (ret < 0)
		return ret;

	SceDisplayFrameBuf framebuf = {
		.size = sizeof(framebuf),
		.base = fb_base,
		.pitch = SCREEN_STRIDE,
		.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
		.width = SCREEN_WIDTH,
		.height = SCREEN_HEIGHT,
	};

	return sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

int debug_screen_finish()
{
	return sceKernelFreeMemBlock(fb_memuid);
}

void debug_screen_clear(unsigned int color)
{
	draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);
}

void debug_screen_draw_char(int x, int y, unsigned int color, char c)
{
	int i, j;
	unsigned char *glyph = &msx_font[c * 8];

	if (c < 0x20 || c > 0x7E)
		return;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			unsigned int c = BG_COLOR;

			if (*glyph & (128 >> j))
				c = color;

			draw_rectangle(x + 2 * j, y + 2 * i, 2, 2, c);
		}
		glyph++;
	}
}

void debug_screen_draw_string(int x, int y, unsigned int color, const char *s)
{
	int start_x = x;

	while (*s) {
		if (*s == '\n') {
			x = start_x;
			y += GLYPH_SIZE;
		} else if (*s == '\t') {
			x += 4 * GLYPH_SIZE;
		} else {
			debug_screen_draw_char(x, y, color, *s);
			x += GLYPH_SIZE;
		}

		if (x + GLYPH_SIZE >= SCREEN_WIDTH)
			x = start_x;

		if (y + GLYPH_SIZE >= SCREEN_HEIGHT)
			y = 0;

		s++;
	}
}

void debug_screen_draw_stringf(int x, int y, unsigned int color, const char *s, ...)
{
	char buf[256];
	va_list argptr;

	va_start(argptr, s);
	vsnprintf(buf, sizeof(buf), s, argptr);
	va_end(argptr);

	debug_screen_draw_string(x, y, color, buf);
}
