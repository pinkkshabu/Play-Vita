#ifndef DEBUG_SCREEN_H
#define DEBUG_SCREEN_H

#define SCREEN_WIDTH    960
#define SCREEN_HEIGHT   544
#define SCREEN_STRIDE   960

#define RGBA8(r, g, b, a)	((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | \
				 (((g) & 0xFF) <<  8) | (((r) & 0xFF) <<  0))

#define COLOR_RED			RGBA8(255, 0,   0,   255)
#define COLOR_GREEN			RGBA8(0,   255, 0,   255)
#define COLOR_BLUE			RGBA8(0,   0,   255, 255)
#define COLOR_YELLOW			RGBA8(255, 255, 0,   255)
#define COLOR_CYAN			RGBA8(0,   255, 255, 255)
#define COLOR_PINK			RGBA8(255, 0,   255, 255)
#define COLOR_LIME			RGBA8(50,  205, 50,  255)
#define COLOR_PURPLE			RGBA8(147, 112, 219, 255)
#define COLOR_ORANGE			RGBA8(255, 128, 0,   255)
#define COLOR_WHITE			RGBA8(255, 255, 255, 255)
#define COLOR_BLACK			RGBA8(0,   0,   0,   255)

int debug_screen_init();
int debug_screen_finish();

void debug_screen_clear(unsigned int color);
void debug_screen_draw_char(int x, int y, unsigned int color, char c);
void debug_screen_draw_string(int x, int y, unsigned int color, const char *s);
void debug_screen_draw_stringf(int x, int y, unsigned int color, const char *s, ...)
	__attribute__((format(printf, 4, 5)));

#endif
